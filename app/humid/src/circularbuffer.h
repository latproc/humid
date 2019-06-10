/*
	All rights reserved. Use of this source code is governed by the
	3-clause BSD License in LICENSE.txt.
*/
#ifndef __CIRCULARBUFFER_H__
#define __CIRCULARBUFFER_H__

#include <inttypes.h>
#include <thread>
#include "Win32Helper.h"
#include "sampletrigger.h"

class CircularBuffer {
public:
	enum DataType { INT16, UINT16, INT32, UINT32, DOUBLE, STR };
	static const int NumTypes = 6;
	class Handler {
	public:
		virtual void operator()() { }
		virtual ~Handler() { }
	};
public:
	CircularBuffer(int size, DataType dt);
	void destroy();
	int size();
	int length();
	double bufferAverage(int n);
	double bufferSum(int n);
	int bufferIndexFor(int i);
	double getBufferValue(int n);
	long getBufferTime(int n);
	void addSample(long time, double val);
	void addSampleDebug(long time, double val);

	/* seek back along the buffer to find the number of samples
	    before a total movement of amount occurred, ignoring direction */
	int findMovement(double amount, int max_len);

	double getBufferValueAt(unsigned long t); /* return an estimate of the value at time t */

	double getTime(int n) ;
	/* calculate the rate of change by a direct method */
	double rate(int n);
	double rateDebug();

	/* calculate the rate of change using a least squares fit (slow) */
	double slope();

	/* find the minimum and maximum values */
	void findRange(double &min_v, double &max_v);
	void findDomain(uint64_t &min_t, int64_t &max_t);
	uint64_t getZeroTime() { return zero_time; } /* first time provided with a sample */
	void setZeroTime(uint64_t t) { zero_time = t; }
	uint64_t getStartTime() { return start_time; } /* time the first sample arrived */

	void freeze() { frozen = true; } /* stop accepting new data */
	void thaw() { frozen = false; } /* start accepting new data */
	bool isFrozen() { return frozen; }

	SampleTrigger *getTrigger(SampleTrigger::Event evt);
	void setTrigger(SampleTrigger *, SampleTrigger::Event evt);

	void setTriggerValue(SampleTrigger::Event evt, int val, SampleTrigger::Kind kind = SampleTrigger::EQUAL);

	void registerCallback(Handler *handler, SampleTrigger::Event evt);
	void deregisterCallback(Handler *handler, SampleTrigger::Event evt);

	DataType getDataType() { return data_type; }
	static DataType dataTypeFromString(const std::string s) {
		if (s == "Signed_int_16") return INT16;
		else if (s == "Signed_int_32") return INT32;
		else if (s == "Ascii_string") return STR;
		else if (s == "Floating_PT_32") return DOUBLE;
		else if (s == "Discrete") return INT16;
		return DOUBLE;
	}
	void clear();

	double smallest() { return smallest_value; }
	double largest() { return largest_value; }

private:
    int bufsize;
    int front;
    int back;
    double total;
    double *values;
	double smallest_value;
	double largest_value;
    uint64_t *times;
	uint64_t zero_time;
	uint64_t start_time;
#ifdef MINGW_USE_BOOST_MUTEX
	boost::recursive_mutex update_mutex;
#else
    std::recursive_mutex update_mutex;
#endif
	//boost::recursive_mutex update_mutex;
	bool frozen;
	SampleTrigger *start_trigger;
	SampleTrigger *stop_trigger;
	DataType data_type;

	std::multimap<SampleTrigger::Event, Handler *> handlers;

};

#endif
