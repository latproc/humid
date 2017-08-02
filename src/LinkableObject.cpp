//
//  LinkableObject.cpp
//  Project: humid
//
//	All rights reserved. Use of this source code is governed by the
//	3-clause BSD License in LICENSE.txt.

#include <iostream>
#include <nanogui/progressbar.h>
#include <nanogui/button.h>
#include "LinkableObject.h"
#include "EditorObject.h"
#include "EditorWidget.h"
#include "EditorButton.h"
#include "EditorProgressBar.h"

extern std::string shortName(const std::string s);


std::ostream &LinkableObject::operator<<(std::ostream &out) const {
    return out;
}
std::ostream &operator<<(std::ostream &out, const LinkableObject &m) {
    return m.operator<<(out);
}

void LinkableObject::update(const Value &v) {
  assert(false);
}

LinkableObject::~LinkableObject() {
    nanogui::Widget *w = dynamic_cast<nanogui::Widget *>(widget);
    if (w) w->decRef();
}

LinkableObject::LinkableObject(EditorObject *ew) : widget(ew) {
    nanogui::Widget *w = dynamic_cast<nanogui::Widget *>(widget);
    if (w) w->incRef();
}

LinkableText::LinkableText(EditorObject *w) : LinkableObject(w) { }

LinkableNumber::LinkableNumber(EditorObject *w) : LinkableObject(w) { }
void LinkableNumber::update(const Value &value) {
    EditorProgressBar *pb = dynamic_cast<EditorProgressBar*>(widget);
    if (pb) {
        if (value.kind == Value::t_integer)
            pb->setValue(value.iValue);
        else if (value.kind == Value::t_float)
            pb->setValue(value.fValue);
    }
}

LinkableIndicator::LinkableIndicator(EditorObject *w) : LinkableObject(w) { }
void LinkableIndicator::update(const Value &value) {
    EditorButton *tb = dynamic_cast<EditorButton*>(widget);
    if (tb) {
        if ( value.kind == Value::t_bool ) {
            tb->setPushed(value.bValue);
        }
        else if (value.kind == Value::t_integer) {
            tb->setPushed(value.iValue == 0);
        }
    }
}
