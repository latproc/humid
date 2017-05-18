/*
	LinePlot.h -- Handle widget with mouse control
	
	This file is based on controls in the NanoGUI souce.

	Please refer to the NanoGUI Licence file NonoGUI-LICENSE.txt

	All rights reserved. Use of this source code is governed by the
	3-clause BSD License in LICENSE.txt.
*/

#pragma once

#include <list>
#include <nanogui/widget.h>
#include <string>
#include <mutex>
#include <map>
#include "SampleTrigger.h"

class CircularBuffer;
class UserWindow;

NAMESPACE_BEGIN(nanogui)

class TimeSeries {
public:
	enum LineStyle { SOLID, POINTS };
	TimeSeries();
	TimeSeries(const std::string title, CircularBuffer *data);
	static const uint64_t TIME_ANY = 0;
	CircularBuffer *getData() const { return data; }
	const std::string &getName() const { return name; }

	void setColor( Color &new_color) { mColor = new_color; }
	Color &getColor() { return mColor; }

	void setScale(float x, float y); // use 0.0 for auto scaled dimensions
	//void setOffset(float x, float y) { h_offset = x; v_offset = y; }
	void getScale(float &x, float &y);
	float getXScale();
	float getYScale();
	//void getOffset(float &x, float &y) { x = h_offset; y = v_offset; }
	//float getXOffset() { return h_offset; }
	float getYOffset() { return v_offset; }
	uint64_t getZeroTime() { return zero_time; }
	uint64_t t0() { return app_zero_time; }

	void drawLabel(NVGcontext *ctx, Vector2i &pos, float val);
	void draw(NVGcontext *ctx, Vector2i &pos, Vector2i &siz, uint64_t x_min, uint64_t x_max, float x_scale, bool show_name = true);

	void setLineWidth(float width) { line_width = width; }
	float getLineWidth() const { return line_width; }
	void setLineStyle(enum LineStyle style) { line_style = style; }
	enum LineStyle getLineStyle() { return line_style; }

private:
	void updateScale();

	std::string name;
	Color mColor;
	CircularBuffer *data;
	//TBD uint64_t t_start;
	//TBD uint64_t t_end;
	float min_v; // smallest V seen so far (for scaling);
	float max_v; // largest V seen so far (for scaling);
	float v_scale;	// vertical scaling factor
	float h_scale;	// horizontal scaling factor
	float v_offset; // vertical offset to the trace after scaling
	//float h_offset; // horizontal offset to the trace after scaling
	//TBD bool frozen; // trace is frozen at start time
	//TBD float x_min_idx; // start the x domain at the timestamp at this index
	//bool auto_scale_v; // automatically scale the data range
	//TBD bool auto_scale_h; // automatically scale the data to the range 0..y_max
	uint64_t app_zero_time;
	uint64_t zero_time;
	float line_width;
	LineStyle line_style;
	// streaming to file
	std::string output_filename;
};

/**
 * \class LinePlot lineplot.h nanogui/lineplot.h
 *
 * \brief Lineplot widget for showing a line plot based on sampled values.
 * 		Used to display a time series.
 */
class NANOGUI_EXPORT LinePlot : public Widget {
public:
    LinePlot(Widget *parent, const std::string &caption = "Untitled");

	std::string monitors();
	void setMonitors(UserWindow *user_window, std::string items_to_monitor);

    const std::string &caption() const { return mCaption; }
    void setCaption(const std::string &caption) { mCaption = caption; }

    const std::string &header() const { return mHeader; }
    void setHeader(const std::string &header) { mHeader = header; }

    const std::string &footer() const { return mFooter; }
    void setFooter(const std::string &footer) { mFooter = footer; }

    const Color &backgroundColor() const { return mBackgroundColor; }
    void setBackgroundColor(const Color &backgroundColor) { mBackgroundColor = backgroundColor; }

    const Color &foregroundColor() const { return mForegroundColor; }
    void setForegroundColor(const Color &foregroundColor) { mForegroundColor = foregroundColor; }

    const Color &textColor() const { return mTextColor; }
    void setTextColor(const Color &textColor) { mTextColor = textColor; }

    const VectorXf &values() const { return mValues; }
    VectorXf &values() { return mValues; }
    void setValues(const VectorXf &values) { mValues = values; }

    virtual Vector2i preferredSize(NVGcontext *ctx) const override;
	void drawGrid(NVGcontext *ctx, int x_start, double x_step, int num_x_steps, int y_start, double y_step,  int num_y_steps);
    virtual void draw(NVGcontext *ctx) override;

    virtual void save(Serializer &s) const override;
    virtual bool load(Serializer &s) override;

	TimeSeries *getTimeSeries(std::string name);
	void addTimeSeries(TimeSeries *);

	// adding a buffer directly creates a default time series to hold it
	void addBuffer(const std::string name, CircularBuffer *);
	void removeBuffer(CircularBuffer *);
	void removeBuffer(const std::string name);

	void setMinX(uint64_t min) { x_min = min; }
	void setTimeScale(float scale) { x_scale = scale; }

	void setMasterSeries(TimeSeries *series); // the master series controls the x range

	std::list<TimeSeries*>getSeries() const { return data; }
	bool scrollEvent(const Vector2i &p, const Vector2f &rel) override;
	bool handleKey(int key, int scancode, int action, int modifiers);
	bool mouseMotionEvent(const nanogui::Vector2i &p, const nanogui::Vector2i &rel, int button, int modifiers) override;

	void freeze();
	void thaw();
	void update(); // check triggers and update state

	void setBufferSize(unsigned int buf_size) { buffer_size = buf_size; }

	void resized(); // called when the widget is resized

	std::string getName() const { return name; }
	void setName(const std::string n) { name = n; }

	void saveData(const std::string fname);

protected:
    std::string mCaption, mHeader, mFooter;
	std::string name;
    Color mBackgroundColor, mForegroundColor, mTextColor;
    VectorXf mValues;  
	std::list<TimeSeries*>data;
	bool overlay_plots; // if true, all plots are scaled vertically to fill the area
	uint64_t start_time; // zero time for this plotter
	uint64_t x_min; // minimum x value for plots
	float x_scale; // horizontal scale applied to all plots
	float x_scroll; // horizontal offset applied to all plots
	TimeSeries *master_series;
	std::recursive_mutex series_mutex;
	bool frozen;
	uint64_t freeze_time_ms;
	unsigned int buffer_size;
	float grid_intensity;
	bool display_grid;
};

NAMESPACE_END(nanogui)
