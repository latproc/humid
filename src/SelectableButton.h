/*
	All rights reserved. Use of this source code is governed by the
	3-clause BSD License in LICENSE.txt.
*/

#ifndef __SelectableButton_h__
#define __SelectableButton_h__

#include <ostream>
#include <string>
#include <nanogui/button.h>
#include "Selectable.h"
#include "UIItem.h"

class SelectableButton : public nanogui::Button, public Selectable, public UIItem {
public:
    SelectableButton(const SelectableButton &orig);
    SelectableButton &operator=(const SelectableButton &other);
    std::ostream &operator<<(std::ostream &out) const;
    bool operator==(const SelectableButton &other);
    
	SelectableButton(const std::string kind, Palette *pal, nanogui::Widget *parent,
			   const std::string &caption = "Untitled");

	virtual bool mouseButtonEvent(const nanogui::Vector2i &p, int button, bool down, int modifiers) override;
	virtual void draw(NVGcontext *ctx) override;
	virtual nanogui::Widget *create(nanogui::Widget *container) const;
private:
	std::string display_caption;
};



std::ostream &operator<<(std::ostream &out, const SelectableButton &m);

#endif
