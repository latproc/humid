//
//  EditorLabel.h
//  Project: humid
//
//	All rights reserved. Use of this source code is governed by the
//	3-clause BSD License in LICENSE.txt.

#ifndef __EditorLabel_h__
#define __EditorLabel_h__

#include <ostream>
#include <string>
#include <nanogui/label.h>
#include "editorwidget.h"

class EditorLabel : public nanogui::Label, public EditorWidget {

public:

	EditorLabel(NamedObject *owner, Widget *parent, const std::string nam,
				LinkableProperty *lp, const std::string caption,
				const std::string &font = "sans", int fontSize = -1, int icon = 0);

	virtual nanogui::Widget *asWidget() override { return this; }
	virtual bool mouseButtonEvent(const nanogui::Vector2i &p, int button, bool down, int modifiers) override;

	virtual bool mouseMotionEvent(const nanogui::Vector2i &p, const nanogui::Vector2i &rel, int button, int modifiers) override;

	virtual bool mouseEnterEvent(const Vector2i &p, bool enter) override;

	virtual void getPropertyNames(std::list<std::string> &names) override;
	void loadProperties(PropertyFormHelper* properties) override;
  virtual void loadPropertyToStructureMap(std::map<std::string, std::string> &property_map) override;
	virtual Value getPropertyValue(const std::string &prop) override;
	virtual void setProperty(const std::string &prop, const std::string value) override;
	virtual void draw(NVGcontext *ctx) override;

  const nanogui::Color &backgroundColor() { return mBackgroundColor; }

  /// Sets the background color of this Button.
  void setBackgroundColor(const nanogui::Color &backgroundColor) { mBackgroundColor = backgroundColor; }

	nanogui::DragHandle *dh;
	std::vector<Handle> handles;
	MatrixXd handle_coordinates;
protected:
  nanogui::Color mBackgroundColor;
	int alignment;
	int valign;
	bool wrap_text;
};

#endif
