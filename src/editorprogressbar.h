//
//  EditorProgressBar.h
//  Project: humid
//
//	All rights reserved. Use of this source code is governed by the
//	3-clause BSD License in LICENSE.txt.

#ifndef __EditorProgressBar_h__
#define __EditorProgressBar_h__

#include <ostream>
#include <string>
#include <map>

#include <nanogui/common.h>
#include <nanogui/widget.h>
#include <nanogui/progressbar.h>

#include "linkableobject.h"
#include "editorwidget.h"
#include "linkableproperty.h"

class EditorProgressBar : public nanogui::ProgressBar, public EditorWidget {

public:

	EditorProgressBar(NamedObject *owner, Widget *parent, const std::string nam, LinkableProperty *lp);

	virtual bool mouseButtonEvent(const nanogui::Vector2i &p, int button, bool down, int modifiers) override;

	virtual bool mouseMotionEvent(const nanogui::Vector2i &p, const nanogui::Vector2i &rel, int button, int modifiers) override;

	virtual bool mouseEnterEvent(const Vector2i &p, bool enter) override;

	virtual void getPropertyNames(std::list<std::string> &names) override;
	void loadProperties(PropertyFormHelper* properties) override;
	virtual void loadPropertyToStructureMap(std::map<std::string, std::string> &property_map) override;
	virtual Value getPropertyValue(const std::string &prop) override;

	virtual void draw(NVGcontext *ctx) override;

	nanogui::DragHandle *dh;
	std::vector<Handle> handles;
	MatrixXd handle_coordinates;
};

#endif
