/*
	All rights reserved. Use of this source code is governed by the
	3-clause BSD License in LICENSE.txt.
*/

#include <iostream>
#include "SelectableButton.h"
#include <nanogui/opengl.h>
#include <nanogui/glutil.h>
#include <nanovg_gl.h>

SelectableButton::SelectableButton(const std::string kind, Palette *pal,
				nanogui::Widget *parent,
				 const std::string &caption)
: nanogui::Button(parent, caption), Selectable(pal), UIItem(kind), display_caption(caption) {
	setButton(this);
}

bool SelectableButton::mouseButtonEvent(const nanogui::Vector2i &p, int button, bool down, int modifiers) {

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
void SelectableButton::draw(NVGcontext *ctx) {
	nanogui::Button::draw(ctx);
	if (mSelected) {
		nvgStrokeWidth(ctx, 4.0f);
		nvgBeginPath(ctx);
		nvgRect(ctx, mPos.x() - 0.5f, mPos.y() - 0.5f, mSize.x() + 1, mSize.y() + 1);
		nvgStrokeColor(ctx, nvgRGBA(80, 220, 0, 255));
		nvgStroke(ctx);
	}
}

nanogui::Widget *SelectableButton::create(nanogui::Widget *container) const {
	return 0;
}


#if 0
SelectableButton::SelectableButton(const SelectableButton &orig){
    text = orig.text;
}

SelectableButton &SelectableButton::operator=(const SelectableButton &other) {
    text = other.text;
    return *this;
}

std::ostream &SelectableButton::operator<<(std::ostream &out) const  {
    out << text;
    return out;
}

std::ostream &operator<<(std::ostream &out, const SelectableButton &m) {
    return m.operator<<(out);
}

bool SelectableButton::operator==(const SelectableButton &other) {
    return text == other.text;
}
#endif

