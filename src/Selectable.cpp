/*
	All rights reserved. Use of this source code is governed by the
	3-clause BSD License in LICENSE.txt.
*/

#include <iostream>
#include "Selectable.h"
#include "Palette.h"
#include "SelectableWidget.h"
#include "SelectableButton.h"

Selectable::Selectable(Palette *pal)
: palette(pal), mSelected(false), widget(0), button(0)
{
}
Selectable::~Selectable() {

}


bool Selectable::isSelected() { return mSelected; }
void Selectable::select() {
	mSelected = true;
	if (palette) palette->select(this);
	justSelected();
}
void Selectable::deselect() {
	mSelected = false;
	if (palette) palette->deselect(this);
	justDeselected();
}
nanogui::Widget *Selectable::getWidget() const {
	return widget;
}
nanogui::Button *Selectable::getButton() const {
	return button;
}
void Selectable::justSelected() { }
void Selectable::justDeselected() { }
void Selectable::setWidget(SelectableWidget *w) {
	widget = w->getWidget();
}
void Selectable::setButton(SelectableButton *w) {
	button = w->getButton();
}

#if 0
Selectable::Selectable(const Selectable &orig){
    text = orig.text;
}

Selectable &Selectable::operator=(const Selectable &other) {
    text = other.text;
    return *this;
}

std::ostream &Selectable::operator<<(std::ostream &out) const  {
    out << text;
    return out;
}

std::ostream &operator<<(std::ostream &out, const Selectable &m) {
    return m.operator<<(out);
}

bool Selectable::operator==(const Selectable &other) {
    return text == other.text;
}
#endif

