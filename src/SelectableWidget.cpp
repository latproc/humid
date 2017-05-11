/*
	All rights reserved. Use of this source code is governed by the
	3-clause BSD License in LICENSE.txt.
*/

#include <iostream>
#include <string>
#include "SelectableWidget.h"
#include <nanogui/opengl.h>
#include <nanogui/glutil.h>
#include <nanovg_gl.h>

SelectableWidget::SelectableWidget(const std::string kind, Palette *pal, nanogui::Widget *parent,
				 const std::string &caption)
: nanogui::Widget(parent), Selectable(pal), UIItem(kind), display_caption(caption) {
	setWidget(this);
}

SelectableWidget::~SelectableWidget() { }

bool SelectableWidget::mouseButtonEvent(const nanogui::Vector2i &p, int button, bool down, int modifiers) {

	using namespace nanogui;

	//if (editorMouseButtonEvent(this, p, button, down, modifiers))
	//	return nanogui::Button::mouseButtonEvent(p, button, down, modifiers);
	//else
	//	if (down && EDITOR->selector()) //EDITOR->selector()->select(this);
	if (this->contains(p)) {
		if (down) {
			if (!mSelected) select(); else deselect();
			return true;
		}
	}
	return false;
}
void SelectableWidget::draw(NVGcontext *ctx) {
	nanogui::Widget::draw(ctx);
	if (mSelected) {
		nvgStrokeWidth(ctx, 2.0f);
		nvgBeginPath(ctx);
		nvgRect(ctx, mPos.x() - 0.5f, mPos.y() - 0.5f, mSize.x() + 1, mSize.y() + 1);
		nvgStrokeColor(ctx, nvgRGBA(80, 220, 0, 255));
		nvgStroke(ctx);
	}
}

#if 0
SelectableWidget::SelectableWidget(const SelectableWidget &orig){
    text = orig.text;
}

SelectableWidget &SelectableWidget::operator=(const SelectableWidget &other) {
    text = other.text;
    return *this;
}

std::ostream &SelectableWidget::operator<<(std::ostream &out) const  {
    out << text;
    return out;
}

std::ostream &operator<<(std::ostream &out, const SelectableWidget &m) {
    return m.operator<<(out);
}

bool SelectableWidget::operator==(const SelectableWidget &other) {
    return text == other.text;
}
#endif

