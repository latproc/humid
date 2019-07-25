/*
	LinePlot.cpp -- Handle widget with mouse control

	This file is based on controls in the NanoGUI souce.

	Please refer to the NanoGUI Licence file NonoGUI-LICENSE.txt

	All rights reserved. Use of this source code is governed by the
	3-clause BSD License in LICENSE.txt.
*/

#include <nanogui/common.h>
#include <nanogui/theme.h>
#include <nanogui/opengl.h>
#include <fstream>
#if 0
#include <nanogui/serializer/core.h>
#endif
#include "circularbuffer.h"
#include "lineplot.h"
#include <value.h>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <nanogui/button.h>
#include <nanogui/label.h>
#include <nanogui/layout.h>
#include <cw_boost_dep.hpp>
#include "userwindow.h"

#include <iostream>

extern CircularBuffer *createBuffer(const std::string name);
NAMESPACE_BEGIN(nanogui)

TimeSeries::TimeSeries() : mColor(255, 255), data(0), /*t_start(TIME_ANY), t_end(TIME_ANY),*/
		min_v(0.0), max_v(1.0), v_scale(1.0), h_scale(1.0), v_offset(0.0), /*h_offset(0.0), frozen(false)*/
	app_zero_time(0), zero_time(0), line_width(2.0f), line_style(tsSOLID)
{ }

TimeSeries::TimeSeries(const std::string title, CircularBuffer *buf)
: name(title), mColor(255, 255), data(buf), /*t_start(TIME_ANY), t_end(TIME_ANY),*/
	v_scale(1.0), h_scale(1.0), v_offset(0.0), /*h_offset(0.0), , frozen(false)*/
	line_width(2.0f), line_style(tsSOLID)
{

}

void TimeSeries::getScale(float &x, float &y) {
	x = h_scale; y = v_scale;
}

float TimeSeries::getXScale() {
	return h_scale;
}

float TimeSeries::getYScale() {
//	if (auto_scale_v)
	updateScale();
	return v_scale;
}

void TimeSeries::updateScale() {
	if (data->length() > 0) {
		double minv = data->getBufferValue(0);
		double maxv = minv;
		size_t n = data->length();
		for (size_t i = 1; i < n; ++i) {
			double y = data->getBufferValue(i);
			if (y < minv) minv = y;
			else if (y>maxv) maxv = y;
		}
		if (minv < min_v) min_v = minv;
		if (maxv > max_v) max_v = maxv;
	}
	v_scale = max_v - min_v;
	if (v_scale <= 0) v_scale = 1.0f;
}

LinePlot::LinePlot(Widget *parent, const std::string &caption)
    : Widget(parent), mCaption(caption), overlay_plots(false),
	start_time(microsecs()), x_min(0), x_scale(1.0), x_scroll(0.0), frozen(false),
	buffer_size(200), grid_intensity(0.05), display_grid(true)
{
    mBackgroundColor = Color(20, 128);
    mForegroundColor = Color(255, 192, 0, 128);
    mTextColor = Color(240, 192);
/*
	setLayout(new GridLayout(Orientation::Horizontal,1));
	Label *lbl = new Label(this, "Start Trigger");
	lbl->setVisible(false);
	Button *btn = new Button(this, "Enable");
	btn->setFixedSize(Vector2i(100,24));
	btn->setVisible(false);
	btn = new Button(this, "Reset");
	btn->setFixedSize(Vector2i(100,24));
	btn->setVisible(false);
	lbl = new Label(this, "Stop Trigger");
	lbl->setVisible(false);
	btn = new Button(this, "Enable");
	btn->setFixedSize(Vector2i(100,24));
	btn->setVisible(false);
	btn = new Button(this, "Reset");
	btn->setFixedSize(Vector2i(100,24));
	btn->setVisible(false);
*/
}

void LinePlot::resized() {
	if (width() >= 500 && height() >= 80) {
		for (auto child : children()) {
			child->setVisible(true);
		}
	}
	else {
		for (auto child : children()) {
			child->setVisible(false);
		}
	}
	UserWindow *uw = dynamic_cast<UserWindow*>(parent());
	if (uw)
		uw->getWindow()->performLayout(uw->getNVGContext());
}

Vector2i LinePlot::preferredSize(NVGcontext *) const {
    return Vector2i(180, 45);
}

void LinePlot::addBuffer(const std::string name, CircularBuffer *cb) {
	data.push_back(new TimeSeries(name, cb));
}


TimeSeries *LinePlot::getTimeSeries(std::string name) {
	for (auto series_ptr : data)
		if (series_ptr->getName() == name) return series_ptr;
	return 0;
}

void LinePlot::addTimeSeries(TimeSeries *series) {
	data.push_back(series);
}


void LinePlot::removeBuffer(CircularBuffer *cb) {
	std::list<TimeSeries*>::iterator iter = data.begin();
	while (iter != data.end()) {
		const TimeSeries *series = *iter;
		const CircularBuffer *buf = series->getData();
		if (buf == cb) {
			iter = data.erase(iter);
			delete series;
		}
		else iter++;
	}
}

void TimeSeries::setScale(float x, float y) {
	if (x) h_scale = x;
	if (y) v_scale = y;
}

void LinePlot::setMasterSeries(TimeSeries *series) {
	master_series = series;
}

void TimeSeries::drawLabel(NVGcontext *ctx, Vector2i &pos, float val) {
	char buf[20];
	const char *fmt = "%5.1f";
	if (val<10.0 && val > -10.0) fmt = "%5.3f";
	snprintf(buf, 20, fmt, val);
	nvgFontSize(ctx, 14.0f);
	nvgTextAlign(ctx, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);
	nvgFillColor(ctx, Color(255,255,255,255));
	nvgText(ctx, pos.x(),pos.y(),buf, NULL);
}

static uint64_t last = 0;
static bool display = false;

uint64_t calcX(CircularBuffer *buf, int i) {
	uint64_t xx1 = buf->getTime(i);
	uint64_t xx2 = buf->getZeroTime();
	uint64_t xx3 = buf->getStartTime();
	uint64_t x = xx1 + xx3 - xx2 ;
	return x;
}

void TimeSeries::draw(NVGcontext *ctx, Vector2i &pos, Vector2i &size, const Axis &y_axis,
					  uint64_t x_min, uint64_t x_max, float x_scale, bool show_name) {
	CircularBuffer *buf = getData();
	int height = size.y();
	nvgFontFace(ctx, "sans");

	if (last == 0) last = microsecs()/1000;
	uint64_t now = microsecs() / 1000;
	display = (now - last > 10000);
    display = true;

	if (show_name && getName().length()) {
		nvgFontSize(ctx, 16.0f);
		nvgTextAlign(ctx, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);
		nvgFillColor(ctx, Color(255,255,255,255));
		nvgText(ctx, pos.x() + 3,
				pos.y() + getYOffset() + (double)height/2.0,
				getName().c_str(), NULL);
	}
	if (buf && buf->length() > 0) {
		double y_scale = 0.9 * height / y_axis.range();
		double y_offset = 0.05 * height;

		{
			Vector2i lbl_pos(pos.x() + 3,pos.y() + getYOffset() + 0.95*height);
			drawLabel(ctx, lbl_pos, y_axis.min());
		}
		{
			Vector2i lbl_pos(pos.x() + 3,pos.y() + getYOffset() + 0.05*height);
			drawLabel(ctx, lbl_pos, y_axis.max());
		}

		size_t n = buf->length();
		//find start index for a given time
		uint64_t i = 0;
		uint64_t x = 0;
		float vy = 0.0f;
		while (i<n) {
			x = buf->getTime(i);
			x -= buf->getZeroTime();
			x += buf->getStartTime();
            // std::cout << "xmin?(" << x << ", " << x_min << "): " << (x<x_min) << "\n";
			if ( x < x_min ) {
                // std::cout << "---------------------\n";
				//if (i==0)
				//	std::cout << "sample too old (x: " << x << " < " << x_min << ")\n";
				break;
			}
			++i;
		}
		// if (display) std::cout << "lineplot: ";
		nvgBeginPath(ctx);
		nvgStrokeWidth(ctx, line_width);
		// indent the graph in the x direction
		int x_indent = 5;
		x_scale *= 1.0f - (float)x_indent * 2.0f / (float)size.x();
		float vx = x_indent;
		if (line_style == tsSOLID) {
			double y = buf->getBufferValue(i);
			vy =  (double)height - ( (y - y_axis.min()) * y_scale + y_offset);
			if (i == n) { // not enough points to fill the graph..
				x = calcX(buf, i-1);
				vx = (x - x_min) * getXScale() * x_scale + x_indent;
				if (vx > size.x()) vx = size.x();
				if (vx < 0.0) vx = 0.0;
			}
            // std::cout << "start(" << vx << ", " << vy << ")" << "\n";
			nvgMoveTo(ctx, pos.x() + vx, pos.y() + vy);
			// if (display) std::cout << (pos.x() + vx) << "," << pos.y() + vy << " ";
		}

		if (i == n) --i;
		for (; i>0; ) {
			--i;
			x = calcX(buf, i);
			vx = (x - x_min) * getXScale() * x_scale + x_indent;
			if (line_style == tsSOLID)
				nvgLineTo(ctx, pos.x() + vx, pos.y() + vy);

			double y = buf->getBufferValue(i);
			vy =  (double)height - ( (y-y_axis.min()) * y_scale + y_offset);
			if (vy > height) vy = height;
			else if (vy < 0.0) vy = 0.0;
			if (line_style == tsSOLID) {
				nvgLineTo(ctx, pos.x() + vx, pos.y() + vy);
			}
			else {
				float l = line_width/2.0f;
				nvgRect(ctx, pos.x() + vx - l, pos.y() + vy - l, line_width, line_width);
			}
            // std::cout << "p(" << vx << ", " << vy << ")" << " ";
			// if (display) std::cout << (pos.x() + vx) << "," << pos.y() + vy << " ";
		}
        // std::cout << "\n";
		vx = (x_max - x_min) * getXScale() * x_scale + x_indent;

		nvgLineTo(ctx, pos.x() + vx, pos.y() + vy);
        // std::cout << "end(" << vx << ", " << vy << ")" << "\n";
		// if (display) std::cout << (pos.x() + vx) << "," << pos.y() + vy << "\n" << std::flush;
		nvgStrokeColor(ctx, mColor);
		nvgStroke(ctx);
		if (display) {
			last = now;
			display = false;
		}
	}
}

static std::string display_time(double dt) {
	char buf[20];
	if ( fabs(dt) > 60000.0) {
		int min = dt/60000;
		double sec = fabs( (dt-min*60.0)/60000.0 ) * 60.0;
		snprintf(buf, 20, "%02d:%02.3f",min, sec);
	}
	else if ( fabs(dt) > 1000.0) snprintf(buf, 20, "%7.3lfs",dt/1000.0);
	else snprintf(buf, 20, "%5.0lfms",dt);
	return buf;
}

void LinePlot::drawGrid(NVGcontext *ctx, int x_start, double x_step, int num_x_steps, int y_start, double y_step,  int num_y_steps) {
//void LinePlot::drawGrid(ctx, indent, displayed_time_range*time_scale/10.0, 10, mPos.y() + top_indent, (double)height/num_gridlines_v,  num_gridlines_v);
	// plot a grid
	if (!display_grid) return;
	double x = x_start;
	for (int i = 0; i<= num_x_steps; ++i) {
		int xp = mPos.x() + x;
		nvgBeginPath(ctx);
		nvgStrokeWidth(ctx, 0.1);
		nvgMoveTo(ctx, xp, y_start);
		nvgLineTo(ctx, xp, y_start + y_step * num_y_steps);
		nvgStrokeColor(ctx, Color(Vector3f(0,150,0),grid_intensity));
		nvgStroke(ctx);
		x += x_step;
	}
	double y = y_start;
	x = mPos.x() + x_start;
	for (int i=0; i <= num_y_steps; ++i) {
		nvgBeginPath(ctx);
		nvgStrokeWidth(ctx, 0.1);
		nvgMoveTo(ctx, x, y);
		nvgLineTo(ctx, x + x_step * num_x_steps, y);
		nvgStrokeColor(ctx, Color(Vector3f(0,150,0),grid_intensity));
		nvgStroke(ctx);
		y += y_step;
	}
}


void LinePlot::draw(NVGcontext *ctx) {
	Widget::draw(ctx);
	nvgBeginPath(ctx);
	nvgRect(ctx, mPos.x(), mPos.y(), mSize.x(), mSize.y());
	nvgFillColor(ctx, mBackgroundColor);
	nvgFill(ctx);

	int top_indent = 12;
	int separation = 5;
	int height = mSize.y() - 2*top_indent;
	int indent = 5;
	int num_gridlines_v = 10;
	if (data.size()) {
		if (!overlay_plots) {
			height /= data.size();
			num_gridlines_v /= data.size();
		}
	}
	// set the time scale for the plot
	uint64_t now = 0;
 	if (!frozen)
	 	now = (microsecs()+ 500)/1000;
	else
	  	now = freeze_time_ms;
	const int default_time_range = 30000;
	if (x_scale == 0.0) x_scale = 0.001;
	double displayed_time_range = (double) default_time_range / x_scale;
	x_min = now - displayed_time_range * (1 - x_scroll);
	double time_scale = (double)(mSize.x()-2*indent) / displayed_time_range;

	if (display_grid && (overlay_plots || data.size() == 0) )
		drawGrid(ctx, indent, displayed_time_range*time_scale/10.0, 10, mPos.y() + top_indent, (double)height/num_gridlines_v,  num_gridlines_v);

	// plot each series
	int plot_num = 0;
	{
		RECURSIVE_LOCK  lock(series_mutex);
		std::vector<Axis>y_axes(CircularBuffer::NumTypes);
		{
			Axis a;
			for (int i = CircularBuffer::INT16; i<= CircularBuffer::STR; ++i) {
				y_axes[i] = a;
			}
		}

		// calculate y scales
		if (overlay_plots) {
			for (auto *series_ptr : data) {
				CircularBuffer *buf = series_ptr->getData();
				y_axes[buf->getDataType()].add(buf->smallest());
				y_axes[buf->getDataType()].add(buf->largest());
			}
		}

		for (auto *series_ptr : data) {
			int plot_pos = mPos.y() + top_indent;
			if (!overlay_plots) {
				plot_pos += plot_num * (height + separation);
				if (display_grid && !overlay_plots)
					drawGrid(ctx, indent, displayed_time_range * time_scale/10.0, 10,
						plot_pos + top_indent, (double)height/num_gridlines_v,  num_gridlines_v);
			}
			nanogui::Vector2i pos(mPos);
			pos.y() = plot_pos + top_indent;
			nanogui::Vector2i siz(mSize);
			siz.y() = height;
			TimeSeries &series(*series_ptr);
			nvgFillColor(ctx, series.getColor());
			Axis axis(y_axes[series.getData()->getDataType()]);
			if (!overlay_plots) {
				CircularBuffer *buf = series_ptr->getData();
				axis.clear();
				axis.add(buf->smallest());
				axis.add(buf->largest());
			}
			series.draw(ctx, pos, siz, axis, x_min, now, time_scale, !overlay_plots);

			++plot_num;
		}
		// draw key if overlaying plots
		if (overlay_plots) {
			nvgFontSize(ctx, 16.0f);
			nvgTextAlign(ctx, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);
			int x = mPos.x();
			int y = mPos.y() + mSize.y() - 24;
			for (auto *series_ptr : data) {
				TimeSeries &series(*series_ptr);
				nvgFillColor(ctx, series.getColor());
				nvgText(ctx, x,y, series.getName().c_str(), NULL);
				float bounds[4];
				x += nvgTextBounds(ctx, x, y, series.getName().c_str(), nullptr, bounds) + 10;
			}
		}
	}

    nvgFontFace(ctx, "sans");

    //if (!mCaption.empty())
	{
		std::string start( display_time(displayed_time_range * (x_scroll - 1)) );
        nvgFontSize(ctx, 14.0f);
        nvgTextAlign(ctx, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);
        nvgFillColor(ctx, mTextColor);
        nvgText(ctx, mPos.x() + 3, mPos.y() + 1, start.c_str(), NULL);
    }

    //if (!mHeader.empty())
	{
		std::string end( display_time(displayed_time_range * x_scroll) );
        nvgFontSize(ctx, 18.0f);
        nvgTextAlign(ctx, NVG_ALIGN_RIGHT | NVG_ALIGN_TOP);
        nvgFillColor(ctx, mTextColor);
        nvgText(ctx, mPos.x() + mSize.x() - 3, mPos.y() + 1, end.c_str(), NULL);
    }

    if (!mFooter.empty()) {
        nvgFontSize(ctx, 15.0f);
        nvgTextAlign(ctx, NVG_ALIGN_RIGHT | NVG_ALIGN_BOTTOM);
        nvgFillColor(ctx, mTextColor);
        nvgText(ctx, mPos.x() + mSize.x() - 3, mPos.y() + mSize.y() - 1, mFooter.c_str(), NULL);
    }

    nvgBeginPath(ctx);
    nvgRect(ctx, mPos.x(), mPos.y(), mSize.x(), mSize.y());
    nvgStrokeColor(ctx, Color(100, 255));
    nvgStroke(ctx);
}

bool LinePlot::scrollEvent(const Vector2i &p, const Vector2f &rel) {
	if (data.size() == 0) return false;
	nanogui::TimeSeries *ts = data.front();
	if (ts->getData()->length() == 0) return false;
	Eigen::Vector2i mp(p - position());

	float prev_scale = x_scale;

	float wid = width();
	double offs = (float)mp.x() / wid;
	double x_max = x_min + (float)30000 / x_scale;
	double t = x_max - (1.0 - offs) * 30000.0 / x_scale;
	double t_data = t - ts->getData()->getStartTime() + ts->getData()->getZeroTime();
	double val = ts->getData()->getBufferValueAt(t_data);
	char buf[80];
	snprintf(buf, 80, "%s,%5.2lf", display_time(t_data).c_str(), val);
	std::cout << buf << "\n";
	setTooltip(buf);

	x_scale += rel.y()/2.0;
	if (x_scale <0.1) x_scale = 0.1;
	std::cout << "scroll: " << rel.y() << "x scale: " << x_scale << "\n";

	double offs2 = (x_max - t) * x_scale / 30000.0;
	double t2 = x_max - (1.0 - offs) * 30000.0 / x_scale;
	double dt = (t2 - t);
	double scroll_change = dt / 30000.0 * x_scale;
	x_scroll -= scroll_change;

	return false;
}

bool LinePlot::mouseMotionEvent(const nanogui::Vector2i &p, const nanogui::Vector2i &rel, int button, int modifiers) {
	if (data.size() == 0) return Widget::mouseMotionEvent(p, rel, button, modifiers);
	std::string tooltip;
	const char *sep = "";
	for (auto ts : data){
		if (ts->getData()->length() == 0) return Widget::mouseMotionEvent(p, rel, button, modifiers);;
		Eigen::Vector2i mp(p - position());

		float wid = width();
		double offs = (float)mp.x() / wid;
		double x_max = (double)x_min + 30000.0 / x_scale;
		double t = x_max - (1.0 - offs) * 30000.0 / x_scale;
		double t_data = t - ts->getData()->getStartTime() + ts->getData()->getZeroTime();
		double val = ts->getData()->getBufferValueAt(t_data);
		char buf[80];
		snprintf(buf, 80, "%s%s: %5.2lf", sep,ts->getName().c_str(), /* display_time(t_data).c_str(), */ val);
		sep = "\n";
		tooltip += buf;
	}
	setTooltip(tooltip);

	return Widget::mouseMotionEvent(p, rel, button, modifiers);
}

bool LinePlot::handleKey(int key, int scancode, int action, int modifiers) {
	if (action == GLFW_PRESS || action == GLFW_REPEAT) {
		if (key == GLFW_KEY_LEFT) {
			if (!frozen && x_scroll == 0.0) freeze();
			x_scroll -= 1.0 / 20.0;
		}
		else if (key == GLFW_KEY_RIGHT) {
			x_scroll += 1.0 / 20.0;
			if (x_scroll >= 0.0) {
				//x_scroll = 0.0;
			}
		}
		else if (key == GLFW_KEY_SPACE) {
			if (frozen) thaw(); else freeze();
		}
	}
	return true;
}

void LinePlot::freeze() {
	if (frozen) return;
	for (auto *series_ptr : data) {
		series_ptr->getData()->freeze();
	}
	frozen = true;
	freeze_time_ms = (microsecs()+500)/1000;
}

void LinePlot::thaw() {
	for (auto *series_ptr : data) {
		if (series_ptr->getData())
			series_ptr->getData()->thaw();
	}
	frozen = false;
	x_scroll = 0.0;
}

void LinePlot::update() {
}

#if 0
void LinePlot::save(Serializer &s) const {
    Widget::save(s);
    s.set("caption", mCaption);
    s.set("header", mHeader);
    s.set("footer", mFooter);
    s.set("backgroundColor", mBackgroundColor);
    s.set("foregroundColor", mForegroundColor);
    s.set("textColor", mTextColor);
    s.set("values", mValues);
}

bool LinePlot::load(Serializer &s) {
    if (!Widget::load(s)) return false;
    if (!s.get("caption", mCaption)) return false;
    if (!s.get("header", mHeader)) return false;
    if (!s.get("footer", mFooter)) return false;
    if (!s.get("backgroundColor", mBackgroundColor)) return false;
    if (!s.get("foregroundColor", mForegroundColor)) return false;
    if (!s.get("textColor", mTextColor)) return false;
    if (!s.get("values", mValues)) return false;
    return true;
}
#endif

std::string LinePlot::monitors() {
	RECURSIVE_LOCK  lock(series_mutex);
	std::string res;
	std::list<TimeSeries*>::const_iterator iter = data.begin();
	while (iter != data.end()) {
		const TimeSeries *ts = *iter++;
		res += ts->getName();
		if (iter != data.end()) res += ",";
	}
	return res;
}

void LinePlot::setMonitors(UserWindow *user_window, std::string items_to_monitor) {

	if (!user_window) return;

	RECURSIVE_LOCK  lock(series_mutex);
	for (auto ts : data) delete ts;
	data.clear();

	Color colors[] = {
		Color(200,0,0, 255), Color(0,200,0, 255), Color(0, 200, 200, 255),
		Color(200, 200, 0, 255), Color(200, 0, 200, 255)
	};
	int numColors = 5;

	std::set<std::string> item_names;
	char *s = strdup(items_to_monitor.c_str()), *p = s, *q=s;
	while ( (q = strchr(p, ',')) != 0) {
		*q++ = 0;
		item_names.insert(p);
		p = q;
	}
	if (*p) item_names.insert(p);

	std::map<std::string, CircularBuffer*> &data(user_window->getData());
	for (auto item : item_names) {
		CircularBuffer *buf = user_window->getDataBuffer(item);
		if (!buf) {
			buf = user_window->createBuffer(item);
		}
		if (buf) {
			nanogui::TimeSeries *ts = new nanogui::TimeSeries(item, buf);
			Color &color(colors[ getSeries().size() % numColors ]);
			ts->setColor(color);
			addTimeSeries(ts);
		}
		else std::cerr << "Failed to create time series for unknown object '" << item << "'\n";
	}
	master_series = 0;
}

void LinePlot::saveData(const std::string fname_) {
	if (data.size() == 0) return;
	RECURSIVE_LOCK  lock(series_mutex);
    boost::filesystem::path path_fix(fname_);
    std::string fname = path_fix.string();
	std::ofstream out(fname);
	if (!out.good()) {
		std::cerr << "failed to open file for writing\n";
		return;
	}
	// header
	out << "Time";
	for (auto *ts : data) {
		out << "," << ts->getName();
	}
	out << "\n";

	// deal with the simple case of only one time series
	if (data.size() == 1) {
		TimeSeries *ts = data.front();
		CircularBuffer *buf = ts->getData();
		int n = buf->length();
		while (n-- > 0) {
			out << buf->getTime(n) << "," << buf->getBufferValue(n) << "\n";
		}
		out.close();
		return;
	}

	// perform a merge of all series

	// check whether all series are empty
	uint64_t t = 0;
	for (auto *ts : data) {
		CircularBuffer *buf = ts->getData();
		if (buf->length()) {
			t = buf->getTime( buf->length()-1);
			break;
		}
	}
	if (!t) { out.flush(); out.close(); return; }

	// initialise buffer indexes
	int n = data.size();
	int pos[n];
	double values[n];
	CircularBuffer *bufs[ data.size() ];
	int i = 0;

	// count out how many series have data
	int outstanding = data.size();
	for (auto *ts : data) {
		CircularBuffer *buf = ts->getData();
		pos[i] = buf->length()-1;
		bufs[i] = buf;
		if (pos[i] == -1) --outstanding;
		++i;
	}

	// main loop
	while (outstanding > 0) {
		int i = 0, j = 0;
		// find earliest of remaining columns
		while (j<n && pos[j]==-1) ++j;
		assert(j<n);
		i = j;
		uint64_t earliest = bufs[j]->getTime( pos[j] );
		uint64_t next = earliest;
		while (++j<n) {
			while (j<n && pos[j]==-1) ++j;
			if (j<n) {
				next = bufs[j]->getTime( pos[j] );
				if (next < earliest) {
					i = j;
					earliest = next;
				}
			}
		}

		// output values at current index position and move index on for
		// buffers that have a value at the earliest time.
		j = 0;
		out << earliest;
		while (j < n) {
			out << ",";
			if (pos[j] > -1) {
				uint64_t t = bufs[j]->getTime( pos[j] );
				out << bufs[j]->getBufferValue( pos[j] );
				if (t == earliest) {
					pos[j]--;
					if (pos[j] == -1) --outstanding; // hit the end of this buffer
				}
			}
			else if (bufs[j]->length())
					out << bufs[j]->getBufferValue(0);
			++j;
		}
		out << "\n";
	}
}


NAMESPACE_END(nanogui)
