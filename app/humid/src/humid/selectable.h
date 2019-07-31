/*
	All rights reserved. Use of this source code is governed by the
	3-clause BSD License in LICENSE.txt.
*/

#ifndef __Selectable_h__
#define __Selectable_h__

#include <ostream>
#include <string>
#include <nanogui/widget.h>

class SelectableWidget;
class SelectableButton;
class Palette;

class Selectable {
public:
    Selectable(const Selectable &orig);
    Selectable &operator=(const Selectable &other);
    std::ostream &operator<<(std::ostream &out) const;
    bool operator==(const Selectable &other);

	virtual ~Selectable();

	Selectable(Palette *pal);
	bool isSelected();
	void select();
	void deselect();
	//nanogui::Widget *getWidget() const;
	//nanogui::Button *getButton() const;
	virtual void justSelected();
	virtual void justDeselected();
	//void setWidget(SelectableWidget *w);
	//void setButton(SelectableButton *w);

protected:
	Palette *palette;
	bool mSelected;
	//nanogui::Widget *widget;
	//nanogui::Button *button;
};

std::ostream &operator<<(std::ostream &out, const Selectable &m);

#endif
