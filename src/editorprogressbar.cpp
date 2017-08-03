//
//  EditorProgressBar.cpp
//  Project: humid
//
//	All rights reserved. Use of this source code is governed by the
//	3-clause BSD License in LICENSE.txt.

#include <iostream>
#include <string>
#include <nanogui/common.h>
#include <nanogui/widget.h>
#include "selectable.h"
#include "linkableobject.h"
#include "editor.h"
#include "editorwidget.h"
#include "editorprogressbar.h"

EditorProgressBar::EditorProgressBar(NamedObject *owner, Widget *parent, const std::string nam, LinkableProperty *lp)
	: ProgressBar(parent), EditorWidget(owner, "ProgressBar", nam, this, lp), dh(0), handles(9), handle_coordinates(9,2) {
	}

bool EditorProgressBar::mouseButtonEvent(const nanogui::Vector2i &p, int button, bool down, int modifiers) {
    using namespace nanogui;

    if (editorMouseButtonEvent(this, p, button, down, modifiers))
        return nanogui::ProgressBar::mouseButtonEvent(p, button, down, modifiers);

    return true;
}

bool EditorProgressBar::mouseMotionEvent(const nanogui::Vector2i &p, const nanogui::Vector2i &rel, int button, int modifiers) {

    if (editorMouseMotionEvent(this, p, rel, button, modifiers))
        return ProgressBar::mouseMotionEvent(p, rel, button, modifiers);

    return true;
}

bool EditorProgressBar::mouseEnterEvent(const Vector2i &p, bool enter) {

    if (editorMouseEnterEvent(this, p, enter))
        return ProgressBar::mouseEnterEvent(p, enter);

    return true;
}

void EditorProgressBar::draw(NVGcontext *ctx) {
    nanogui::ProgressBar::draw(ctx);
    if (mSelected) drawSelectionBorder(ctx, mPos, mSize);
}

void EditorProgressBar::loadPropertyToStructureMap(std::map<std::string, std::string> &property_map) {
	EditorWidget::loadPropertyToStructureMap(property_map);
}

void EditorProgressBar::getPropertyNames(std::list<std::string> &names) {
    EditorWidget::getPropertyNames(names);
}

Value EditorProgressBar::getPropertyValue(const std::string &prop) {
	Value res = EditorWidget::getPropertyValue(prop);
  if (res != SymbolTable::Null)
    return res;
  return SymbolTable::Null;
}
