/*
	All rights reserved. Use of this source code is governed by the
	3-clause BSD License in LICENSE.txt.
*/
#include <stdio.h>
#include <inttypes.h>
#include <stdlib.h>
#include <thread>
#include <mutex>
#include "circularbuffer.h"
#include <assert.h>
#include <math.h>
#include <value.h>
#include <map>
#include "sampletrigger.h"

CircularBuffer::CircularBuffer(int size, DataType dt)
{
	bufsize = size;
	front = -1;
	back = -1;
	values = (double*)malloc( sizeof(double) * size);
	times = (uint64_t*)malloc( sizeof(uint64_t) * size);
	total = 0;
	zero_time = 0;
	start_time = 0;
	frozen = false;
	start_trigger = 0;
	stop_trigger = 0;
	data_type = dt;
}

void CircularBuffer::destroy() {
	{
    RECURSIVE_LOCK lock(update_mutex);
    free(values);
    free(times);
	}
	delete this;
}

void CircularBuffer::clear() {
	RECURSIVE_LOCK  lock(update_mutex);
	front = back = -1;
}

int CircularBuffer::bufferIndexFor(int i) {
	RECURSIVE_LOCK  lock(update_mutex);
	int l = length();
	/* it is a non recoverable error to access the buffer without
		adding a value */
	if (l == 0) { assert(0); abort(); }

	if (i>=l) return back; /* looking past the end of the buffer */
	return (front + bufsize - i) % bufsize;
}

void CircularBuffer::addSample(long time, double val) {
	if (frozen) return;
	RECURSIVE_LOCK  lock(update_mutex);

	if (data_type == INT16) val = (int16_t)val;
	else if (data_type == UINT16) val = (uint16_t)val;

	if (zero_time == 0) { zero_time = time; start_time = microsecs() / 1000; }
	if (start_trigger && start_trigger->active()) {
		Value v(val);
		start_trigger->check(v);
		if (start_trigger->justTriggered()) thaw();
	}
	if (back == -1 || val < smallest_value) smallest_value = val;
	if (back == -1 || val > largest_value) largest_value = val;
	front = (front + 1) % bufsize;
	if (front == back) total -= values[front];
	if (front == back || back == -1) back = (back + 1) % bufsize;
	total += val;
	values[front] = val;
	times[front] = time;
	if (stop_trigger && stop_trigger->active()) {
		Value v(val);
		stop_trigger->check(v);
	}
	if (start_trigger && start_trigger->justTriggered()) {
		std::multimap<SampleTrigger::Event, Handler *>::iterator iter = handlers.find(SampleTrigger::START);
		while (iter != handlers.end()) {
			const std::pair<SampleTrigger::Event, Handler *> &item = *iter++;
			if (item.first != SampleTrigger::START) break;
			item.second->operator()();
		}
	}
	if (stop_trigger && stop_trigger->justTriggered()) {
		std::multimap<SampleTrigger::Event, Handler *>::iterator iter = handlers.find(SampleTrigger::STOP);
		while (iter != handlers.end()) {
			const std::pair<SampleTrigger::Event, Handler *> &item = *iter++;
			if (item.first != SampleTrigger::STOP) break;
			item.second->operator()();
		}
	}
}

void CircularBuffer::registerCallback(Handler *handler, SampleTrigger::Event evt) {
	handlers.insert(std::make_pair(evt, handler));
}

void CircularBuffer::deregisterCallback(Handler *handler, SampleTrigger::Event evt) {
	std::multimap<SampleTrigger::Event, Handler *>::iterator iter = handlers.find(evt);
	while (iter != handlers.end()) {
		const std::pair<SampleTrigger::Event, Handler *> &item = *iter;
		if (item.first != evt) break;
		if (item.second == handler) {
			handlers.erase(iter);
			break;
		}
		iter = iter++;
	}
}

void CircularBuffer::addSampleDebug(long time, double val) {
	addSample(val, time);
	printf("buffer added: %5.2f, %ld at %d\n", val, time, front);
}

double CircularBuffer::rate(int n) {
	RECURSIVE_LOCK  lock(update_mutex);
    if (front == back || back == -1) return 0.0f;
	int idx = bufferIndexFor(n);
    double v1 = values[idx], v2 = values[front];
    double t1 = times[idx], t2 = times[front];
    double ds = v2-v1;
    double dt = t2-t1;
		if (dt == 0) return 0;
    return ds/dt;
}

void CircularBuffer::findRange(double &min_v, double &max_v) {
	RECURSIVE_LOCK  lock(update_mutex);
	/* reading the buffer without entering data is a non-recoverable error */
	int l = length();
	if (l == 0) { min_v = max_v = 0.0; }
	int idx = front;
	max_v = values[idx];
	min_v = values[idx];
	while (idx != back) {
		idx--; if (idx == -1) idx = bufsize-1;
		if (max_v < values[idx]) max_v = values[idx];
		if (min_v > values[idx]) min_v = values[idx];
	}
}

int CircularBuffer::findMovement(double amount, int max_len) {
	RECURSIVE_LOCK  lock(update_mutex);
	/* reading the buffer without entering data is a non-recoverable error */
	int l = length();
	if (l == 0) { assert(0); abort(); }
	int n = 0;
	int idx = front;
	double current = values[idx];

	while (idx != back && n<max_len) {
		idx--; if (idx == -1) idx = bufsize-1;
		++n;
		if ( fabs(current - values[idx]) >= amount) return n;
	}
	return  n;
}

double CircularBuffer::rateDebug() {
	RECURSIVE_LOCK  lock(update_mutex);
    if (front == back || back == -1) return 0.0f;
    double v1 = values[back], v2 = values[front];
    double t1 = times[back], t2 = times[front];
    double ds = v2-v1;
    double dt = t2-t1;
		if (dt == 0)
			printf("error calculating rate: (%5.2f - %5.2f)/(%5.2f - %5.2f) = %5.2f / %5.2f \n", v2, v1, t2, t1, ds, dt);
		else
			printf("calculated rate: (%5.2f - %5.2f)/(%5.2f - %5.2f) = %5.2f / %5.2f = %5.2f\n", v2, v1, t2, t1, ds, dt, ds/dt);
		if (dt == 0) return 0;
    return ds/dt;
}

double CircularBuffer::bufferAverage(int n) {
	RECURSIVE_LOCK  lock(update_mutex);
	int l = length();
	if (n>l) n = l;
	return (n==0) ? n : bufferSum(n) / n;
}

double CircularBuffer::getBufferValue(int n) {
	RECURSIVE_LOCK  lock(update_mutex);
	int idx = bufferIndexFor(n);
	return values[ idx ];
}

long CircularBuffer::getBufferTime(int n) {
	RECURSIVE_LOCK  lock(update_mutex);
	int idx = bufferIndexFor(n);
	return times[ idx ];
}

double CircularBuffer::getBufferValueAt(unsigned long t) {
	RECURSIVE_LOCK  lock(update_mutex);
	int n = length() - 1;
	int idx = bufferIndexFor(n);

	if (times[idx] >= t) return values[idx];
	if (t >= times[front]) return values[front];
	int nxt = (idx+1) % bufsize;
	while (times[nxt] < t) { idx = nxt; nxt = (idx+1) % bufsize; }
	return values[nxt];
}

double CircularBuffer::bufferSum(int n) {
	RECURSIVE_LOCK  lock(update_mutex);
	int i = length();
	if (i>n) i = n;
	double tot = 0.0;
	while (i) {
		i--;
		tot = tot + getBufferValue(i);
	}
	return tot;
}

int CircularBuffer::size() {
    return bufsize;
}

int CircularBuffer::length() {
	RECURSIVE_LOCK  lock(update_mutex);
    if (front == -1) return 0;
    return (front - back + bufsize) % bufsize + 1;
}

double CircularBuffer::getTime(int n) {
	RECURSIVE_LOCK  lock(update_mutex);
	return times[ (front + bufsize - n) % bufsize];
}

void CircularBuffer::findDomain(uint64_t &min_t, int64_t &max_t) {
	if (length() == 0) return;
	max_t = getTime(0);
	min_t = getTime(length() - 1);
}

SampleTrigger *CircularBuffer::getTrigger(SampleTrigger::Event evt) {
	switch (evt) {
		case SampleTrigger::Event::START:
			return start_trigger;
			break;
		case SampleTrigger::Event::STOP:
			return stop_trigger;
			break;
		default:;
	}
    return 0;
}

void CircularBuffer::setTrigger(SampleTrigger *t, SampleTrigger::Event evt) {
	switch (evt) {
		case SampleTrigger::Event::START:
			if (start_trigger) delete start_trigger;
			start_trigger = t;
			if (start_trigger) start_trigger->reset();
			break;
		case SampleTrigger::Event::STOP:
			if (stop_trigger) delete stop_trigger;
			stop_trigger = t;
			if (stop_trigger) stop_trigger->reset();
			break;
		default:;
	}
}

void CircularBuffer::setTriggerValue(SampleTrigger::Event evt, int val, SampleTrigger::Kind kind) {
	switch (evt) {
		case SampleTrigger::Event::START:
			start_trigger->setTriggerValue(val);
			break;
		case SampleTrigger::Event::STOP:
			stop_trigger->setTriggerValue(val);
			break;
		default:;
	}
}

double CircularBuffer::slope() {
	RECURSIVE_LOCK  lock(update_mutex);
	double sumX = 0.0, sumY = 0.0, sumXY = 0.0;
	double sumXsquared = 0.0, sumYsquared = 0.0;
	int n = length()-1;
	double t0 = getTime(n);
	for (int i = n-1; i>0; i--) {
			double y = getBufferValue(i) - getBufferValue(n); // degrees
			double x = getTime(i) - t0;
			sumX += x; sumY += y; sumXsquared += x*x; sumYsquared += y*y; sumXY += x*y;
	}
	double denom = (double)n*sumXsquared - sumX*sumX;
	double m = 0.0;
	if (denom != 0.0) m  = ((double)n * sumXY - sumX*sumY) / denom;
	return m;
}

#ifdef TEST
//TBD
int failures = 0;


void fail(int test) {
	++failures;
	printf("test %d failed\n", test);
}

int main(int argc, const char *argv[]) {
	double (*sum)(CircularBuffer *buf, int n) = bufferSum;
	double (*average)(CircularBuffer *buf, int n) = bufferAverage;

  int tests = 0;
  int test_buffer_size = 4;
  struct CircularBuffer *mybuf = createBuffer(test_buffer_size);
  int i = 0;

  for (i=0; i<10; ++i) addSample(mybuf, i, i);
  for (i=0; i<10; ++i) printf("index for %d: %d\n", i, bufferIndexFor(mybuf, i));
	  ++tests; if (rate(mybuf, 6) != 1.0) {
		fail(tests);
		printf("rate returned %.3lf\n", rate(mybuf, 4) );
	}

  for (i=0; i<10; ++i) addSample(mybuf, i, 1.5*i);
	++tests; if (rate(mybuf,8) != 1.5) {
		fail(tests);
		printf("rate returned %.3lf\n", rate(mybuf, 4) );
	}
	++tests; if (slope(mybuf) != 1.5) { fail(tests); }
	++tests; if (length(mybuf) != test_buffer_size) { fail(tests); }
	++tests; if (sum(mybuf, size(mybuf)) / test_buffer_size != average(mybuf,size(mybuf)))
				{ fail(tests); }
	if (test_buffer_size == 4) { /* this test only works if the buffer is of length 4 */
		++tests; if (sum(mybuf, size(mybuf)) != (6 + 7 + 8 + 9)*1.5) { fail(tests); }
	}

	/// negative numbers
	destroyBuffer(mybuf);
	mybuf = createBuffer(test_buffer_size);
	long ave = average(mybuf, test_buffer_size);
	//printf("%ld\n",ave);
	addSample(mybuf,10000,1);
	//printf("%ld\n",ave);
	addSample(mybuf,10040,-20);
	//printf("%ld\n",ave);
	for (i=0; i<10; ++i) addSample(mybuf, i, -1.5*i);
	++tests; if (rate(mybuf, size(mybuf)) != -1.5) {
	fail(tests); printf("unexpected rate: %lf expected -1.5\n", rate(mybuf, size(mybuf))); }

	i=0;
	while (i<test_buffer_size) { addSample(mybuf, i, random()%5000); ++i; }
	while (i<8) { addSample(mybuf, i, 0); ++i; }
	++tests; if ( fabs(bufferAverage(mybuf, size(mybuf))) >1.0E-4) {
	fail(tests); printf("unexpected average: %lf\n", average(mybuf, size(mybuf)));}
	++tests; if (bufferSum(mybuf, size(mybuf)) != 0.0) {
	fail(tests); printf("unexpected sum: %lf\n", bufferSum(mybuf, size(mybuf)));}

	destroyBuffer(mybuf);

	/* test whether findMovement correctly finds a net movememnt */
	++tests;
	mybuf = createBuffer(20);
	addSample(mybuf, 0, 2200);
	addSample(mybuf, 1, 2200);
	for (i=2; i<10; ++i) addSample(mybuf, i, 2000);
	for (; i<20; ++i) addSample(mybuf, i, 2020);
	{ int n;
	if ( ( n=findMovement(mybuf, 10)) != 10 ) {
		fail(tests); printf("findMovement(..,10) expected 10, got %d\n", n);
	}
	++tests;
	if ( ( n=findMovement(mybuf, 100)) != 18 ) {
		/*2200,2200,2000,2000,2000,2000,2000,2000,2000,2000,
		2020,2020,2020,2020,2020,2020,2020,2020,2020,2020 */
		fail(tests); printf("findMovement(..,100) expected 18, got %d\n", n);
	}
	}
	destroyBuffer(mybuf);

	printf("tests:\t%d\nfailures:\t%d\n", tests, failures );

	/*
		generate a sign curve and calculate the slope using
		the rate() and slope() functions for comparison purposes
	*/
  /*
	mybuf = createBuffer(4);
	for (i=0; i<70; i++) {
		double x = i/4.0;
		double y = sin( x );
		addSample(mybuf, i, y);
		if (i>mybufsize) printf("%d,%lf,%lf,%lf\n",i, cos(x)/4.0, rate(mybuf), slope(mybuf));
	}
	destroyBuffer(mybuf);
  */

  mybuf = createBuffer(6);
  for (i=0; i<6; ++i) { double x = i*22+10; double y = i*20 + 30; addSample(mybuf, x, y); }
  for (i=10; i<300; i+=10) { printf ("%4ld\t%8.3f\n", (long)i, getBufferValueAt(mybuf, i)); }
   return 0;
}

#endif
