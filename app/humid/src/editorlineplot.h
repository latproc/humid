//
//  EditorLinePlot.h
//  Project: humid
//
//	All rights reserved. Use of this source code is governed by the
//	3-clause BSD License in LICENSE.txt.

#ifndef __EditorLinePlot_h__
#define __EditorLinePlot_h__

#include <ostream>
#include <string>
#include <lineplot.h>
#include "editorwidget.h"

class EditorLinePlot : public nanogui::LinePlot, public EditorWidget {

public:

	EditorLinePlot(NamedObject *owner, Widget *parent, std::string nam, LinkableProperty *lp = nullptr, int icon = 0);

	virtual nanogui::Widget *asWidget() override { return this; }

	virtual bool mouseButtonEvent(const nanogui::Vector2i &p, int button, bool down, int modifiers) override;

	virtual bool mouseMotionEvent(const nanogui::Vector2i &p, const nanogui::Vector2i &rel, int button, int modifiers) override;

	virtual bool mouseEnterEvent(const Vector2i &p, bool enter) override;

	virtual void getPropertyNames(std::list<std::string> &names) override;
	void loadProperties(PropertyFormHelper* properties) override;
	virtual void loadPropertyToStructureMap(std::map<std::string, std::string> &property_map) override;
	virtual Value getPropertyValue(const std::string &prop) override;

	virtual void draw(NVGcontext *ctx) override;

	void setTriggerName(UserWindow *user_window, SampleTrigger::Event evt, const std::string name);
	void setTriggerValue(UserWindow *user_window, SampleTrigger::Event evt, int val);

private:
	Eigen::MatrixXd handle_coordinates;
	std::string monitored_objects;
	std::string start_trigger_name;
	int start_trigger_value;
	std::string stop_trigger_name;
	int stop_trigger_value;
};

#endif
