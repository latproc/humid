//
//  LinkableObject.cpp
//  Project: humid
//
//	All rights reserved. Use of this source code is governed by the
//	3-clause BSD License in LICENSE.txt.

#include <iostream>
#include <nanogui/progressbar.h>
#include <nanogui/button.h>
#include "linkableobject.h"
#include "editorobject.h"
#include "editorwidget.h"
#include "editorbutton.h"
#include "editorprogressbar.h"
#include "editorimageview.h"
#include "nanogui/textbox.h"
#include "nanogui/label.h"
#include "editor.h"
#include "editortextbox.h"

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

LinkableText::LinkableText(EditorObject *w) : LinkableObject(w) {}

void LinkableText::update(const Value &value) {
	nanogui::TextBox *tb = dynamic_cast<nanogui::TextBox*>(widget);
    if (tb) { tb->setValue(value.asString()); return; }
	nanogui::Label *lbl = dynamic_cast<nanogui::Label*>(widget);
	if (lbl) { lbl->setCaption(value.asString()); return; }
	EditorImageView *iv = dynamic_cast<EditorImageView*>(widget);
	if (iv) {
		iv->setImageName(value.asString()); iv->fit();
		return;
	}
}

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
        if (tb->flags() & nanogui::Button::SetOffButton) {
            if ( value.kind == Value::t_bool ) {
                tb->setPushed(!value.bValue);
            }
            else if (value.kind == Value::t_integer) {
                tb->setPushed(value.iValue == 0);
            }
        }
        else {  
            if ( value.kind == Value::t_bool ) {
                tb->setPushed(value.bValue);
            }
            else if (value.kind == Value::t_integer) {
                tb->setPushed(value.iValue != 0);
            }
            else {
                std::cout << "Linkable Indicator ignoring state " << value << "\n";
            }
        }
    }
}

LinkableVisibility::LinkableVisibility(EditorObject *w) : LinkableObject(w) { }
void LinkableVisibility::update(const Value &value) {
    EditorWidget *ew = dynamic_cast<EditorWidget*>(widget);
    if (ew && ew->asWidget()) {
        if (Editor::instance()->isEditMode())
            ew->asWidget()->setVisible(true);
        else {
            bool vis;
            if (value.asBoolean(vis)) {
                if (!ew->invertedVisibility())
                    ew->asWidget()->setVisible(vis);
                else 
                    ew->asWidget()->setVisible(!vis);
            }
            else ew->asWidget()->setVisible(true);
        }
    }
}

