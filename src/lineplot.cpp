/*
	LinePlot.cpp -- Handle widget with mouse control
	
	This file is based on controls in the NanoGUI souce.

	Please refer to the NanoGUI Licence file NonoGUI-LICENSE.txt

	All rights reserved. Use of this source code is governed by the
	3-clause BSD License in LICENSE.txt.
*/

#include <nanogui/theme.h>
#include <nanogui/opengl.h>
#include <nanogui/serializer/core.h>
#include "circularbuffer.h"
#include "lineplot.h"
#include <value.h>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <nanogui/button.h>
#include <nanogui/label.h>
#include <nanogui/layout.h>
#include "UserWindow.h"

#include <iostream>

NAMESPACE_BEGIN(nanogui)

TimeSeries::TimeSeries() : mColor(255, 255), data(0), /*t_start(TIME_ANY), t_end(TIME_ANY),*/
		min_v(0.0), max_v(1.0), v_scale(1.0), h_scale(1.0), v_offset(0.0), h_offset(0.0)/*, frozen(false)*/,
	app_zero_time(0), zero_time(0), line_width(2.0f), line_style(SOLID)
{ }

TimeSeries::TimeSeries(const std::string title, CircularBuffer *buf)
: name(title), mColor(255, 255), data(buf), /*t_start(TIME_ANY), t_end(TIME_ANY),*/
	v_scale(1.0), h_scale(1.0), v_offset(0.0), h_offset(0.0), /*, frozen(false)*/
	line_width(2.0f), line_style(SOLID)
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
		double maxv = data->getBufferValue(0);
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
}

LinePlot::LinePlot(Widget *parent, const std::string &caption)
    : Widget(parent), mCaption(caption), overlay_plots(false),
	start_time(microsecs()), x_min(0), x_scale(1.0), x_scroll(0.0), frozen(false),
	buffer_size(200)
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
	if (min_v<10.0 && min_v > -10.0) fmt = "%5.3f";
	snprintf(buf, 20, fmt, min_v);
	nvgFontSize(ctx, 14.0f);
	nvgTextAlign(ctx, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);
	nvgFillColor(ctx, Color(255,255,255,255));
	nvgText(ctx, pos.x(),pos.y(),buf, NULL);
}

void TimeSeries::draw(NVGcontext *ctx, Vector2i &pos, Vector2i &size,
					  uint64_t x_min, uint64_t x_max, float x_scale, bool show_name) {
	CircularBuffer *buf = getData();
	int height = size.y();
	nvgFontFace(ctx, "sans");

	if (show_name && getName().length()) {
		nvgFontSize(ctx, 16.0f);
		nvgTextAlign(ctx, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);
		nvgFillColor(ctx, Color(255,255,255,255));
		nvgText(ctx, pos.x() + 3,
				pos.y() + getYOffset() + (double)height/2.0,
				getName().c_str(), NULL);
	}
	if (buf && buf->size() > 0) {

		//buf->findRange(min_v, max_v);
		if (max_v == min_v) max_v += 1.0;
		double y_scale = 0.9 * height / getYScale();
		double y_offset = (0.0 - min_v) * y_scale + getYOffset() + 0.1*height;

		{
			Vector2i lbl_pos(pos.x() + 3,pos.y() + getYOffset() + 0.05*height);
			drawLabel(ctx, lbl_pos, min_v);
		}
		{
			Vector2i lbl_pos(pos.x() + 3,pos.y() + getYOffset() + 0.95*height);
			drawLabel(ctx, lbl_pos, max_v);
		}

		size_t n = buf->length();
		//find start index for a given time
		uint64_t i = 0;
		unsigned long x = 0;
		float vy = height - y_offset;
		while (i<n) {
			x=buf->getTime(i);
			//x -= buf->getZeroTime();
			x += buf->getStartTime();
			if ( x < x_min ) {
				if (i==0)
					std::cout << "sample too old (x: " << x << " < " << x_min << ")\n";
				break;
			}
			++i;
		}
		nvgBeginPath(ctx);
		nvgStrokeWidth(ctx, line_width);
		// indent the graph in the x direction
		int x_indent = 5;
		x_scale *= 1.0f - (float)x_indent * 2.0f / (float)size.x();
		if (line_style == SOLID)
			nvgMoveTo(ctx, pos.x() + x_indent, pos.y());
		float vx = getXOffset() * x_scale + x_indent;
		float last_x = vx;
		for (; i>0; ) {
			if (line_style == SOLID)
				nvgLineTo(ctx, pos.x() + vx, pos.y() + vy);
			long xx1 = buf->getTime(i);
			long xx2 = buf->getZeroTime();
			long xx3 = buf->getStartTime();
			x = xx1 - xx2 + xx3;
			//x = buf->getTime(i) - buf->getZeroTime() + buf->getStartTime();
			if (x < x_min)
				x = x_min;
			if (x > x_max)
				x = x_max;
			--i;
			size_t idx = i;
			if (x >= x_min && x<=x_max) {
				if (i>0) nvgLineTo(ctx, pos.x() + vx, pos.y() + vy);
				vx = ((x - x_min) * getXScale() + getXOffset() )* x_scale + x_indent;
				if (vx > size.x())
					vx = size.x();
				if (vx < 0.0) vx = 0.0;
				if (line_style == SOLID)
					nvgLineTo(ctx, pos.x() + vx, pos.y() + vy);
			}
			double y = buf->getBufferValue(idx);
			vy =  height - (y * y_scale + y_offset);
			if (vy > height) vy = height;
			else if (vy < 0.0) vy = 0.0;
			if (line_style == SOLID)
				nvgLineTo(ctx, pos.x() + vx, pos.y() + vy);
			else {
				float l = line_width/2.0f;
				nvgRect(ctx, pos.x() + vx - l, pos.y() + vy - l, line_width, line_width);
			}
		}
		vx = ((x_max - x_min) * getXScale() + getXOffset() )* x_scale + x_indent;
		if (vx > size.x()) vx = size.x();
		nvgLineTo(ctx, pos.x() + vx, pos.y() + vy);
		nvgStrokeColor(ctx, mColor);
		nvgStroke(ctx);
		//nvgFillColor(ctx, mForegroundColor);
		//nvgFill(ctx);
	}
}

static std::string display_time(double dt) {
	char buf[20];
	if ( fabs(dt) > 60000.0) snprintf(buf, 20, "%7.3lfm",dt/60000.0);
	else if ( fabs(dt) > 1000.0) snprintf(buf, 20, "%7.3lfs",dt/1000.0);
	else snprintf(buf, 20, "%5.0lfms",dt);
	return buf;
}


void LinePlot::draw(NVGcontext *ctx) {
	Widget::draw(ctx);
	nvgBeginPath(ctx);
	nvgRect(ctx, mPos.x(), mPos.y(), mSize.x(), mSize.y());
	nvgFillColor(ctx, mBackgroundColor);
	nvgFill(ctx);

	int top_indent = 12;
	int separation = 5;
	int height = mSize.y() - top_indent;
	if (data.size()) {
		if (!overlay_plots) height /= data.size(); else height -= 30;
	}
	// set the time scale for the plot
	uint64_t now = 0;
 	if (!frozen) now = (microsecs()+ 500)/1000;
	else now = freeze_time_ms;
	const int default_time_range = 30000;
	assert(x_scale > 0.0);
	double displayed_time_range = (double) default_time_range / x_scale;
	x_min = now - displayed_time_range * (1 - x_scroll);
	double time_scale = (double)mSize.x() / displayed_time_range;

	// plot each series
	int plot_num = 0;
	{
		std::lock_guard<std::recursive_mutex>  lock(series_mutex);

		for (auto *series_ptr : data) {
			int plot_pos = mPos.y() + top_indent;
			if (!overlay_plots) plot_pos += plot_num * (height + separation);
			nanogui::Vector2i pos(mPos);
			pos.y() = plot_pos;
			nanogui::Vector2i siz(mSize);
			siz.y() = height;
			TimeSeries &series(*series_ptr);
			series.draw(ctx, pos, siz, x_min, now, time_scale, !overlay_plots);
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
	Eigen::Vector2i mp(p - position());

	float prev_scale = x_scale;

	x_scale += rel.y()/2.0;
	if (x_scale <0.1) x_scale = 0.1;
	std::cout << "scroll: " << rel.y() << "x scale: " << x_scale << "\n";

	x_scroll += 1 - x_scale / prev_scale;

	return false;
}

bool LinePlot::handleKey(int key, int scancode, int action, int modifiers) {
	if (action == GLFW_PRESS || action == GLFW_REPEAT) {
		if (key == GLFW_KEY_LEFT) {
			if (x_scroll == 0.0) freeze();
			x_scroll -= 1.0 / 20.0;
		}
		else if (key == GLFW_KEY_RIGHT) {
			x_scroll += 1.0 / 20.0;
			if (x_scroll >= 0.0) {
				x_scroll = 0.0;
				thaw();
			}
		}
		else if (key == GLFW_KEY_SPACE) {
			if (frozen) thaw(); else freeze();
		}
	}
	return true;
}

void LinePlot::freeze() {
	for (auto *series_ptr : data) {
		series_ptr->getData()->freeze();
	}
	frozen = true;
	freeze_time_ms = microsecs()/1000 - 500;
}

void LinePlot::thaw() {
	for (auto *series_ptr : data) {
		series_ptr->getData()->thaw();
	}
	frozen = false;
	x_scroll = 0.0;
}

void LinePlot::update() {
}


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

std::string LinePlot::monitors() {
	std::lock_guard<std::recursive_mutex>  lock(series_mutex);
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

	std::lock_guard<std::recursive_mutex>  lock(series_mutex);
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
		nanogui::TimeSeries *ts = new nanogui::TimeSeries(item, buf);
		Color &color(colors[ getSeries().size() % numColors ]);
		ts->setColor(color);
		addTimeSeries(ts);
	}
	master_series = 0;
}


NAMESPACE_END(nanogui)
