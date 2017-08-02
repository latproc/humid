//
//  EditorImageView.h
//  Project: humid
//
//	All rights reserved. Use of this source code is governed by the
//	3-clause BSD License in LICENSE.txt.

#ifndef __EditorImageView_h__
#define __EditorImageView_h__

#include <ostream>
#include <string>
#include <nanogui/imageview.h>
#include "editorwidget.h"

class EditorImageView : public nanogui::ImageView, public EditorWidget {

public:

	EditorImageView(Widget *parent, const std::string nam, LinkableProperty *lp, GLuint image_id, int icon = 0);

	virtual bool mouseButtonEvent(const nanogui::Vector2i &p, int button, bool down, int modifiers) override;

	virtual bool mouseMotionEvent(const nanogui::Vector2i &p, const nanogui::Vector2i &rel, int button, int modifiers) override;

	virtual bool mouseEnterEvent(const Vector2i &p, bool enter) override;

	virtual void getPropertyNames(std::list<std::string> &names) override;
	void loadProperties(PropertyFormHelper* properties) override;
	virtual void loadPropertyToStructureMap(std::map<std::string, std::string> &property_map) override;
	virtual Value getPropertyValue(const std::string &prop) override;

	virtual void draw(NVGcontext *ctx) override;

	void setImageName(const std::string new_name);
	const std::string &imageName() const;

	void refresh();

protected:
	nanogui::DragHandle *dh;
	std::vector<Handle> handles;
	MatrixXd handle_coordinates;
	std::string image_name;
	bool need_redraw;
};

#endif
