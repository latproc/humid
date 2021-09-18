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
PropertyLinkTarget::PropertyLinkTarget(EditorWidget *widget, const std::string & property, const Value &default_value)
	: widget_(widget), property_name(property), default_value(default_value) {
		
}

void PropertyLinkTarget::update(const Value &value) {
    if (widget_) {
        widget_->setPropertyValue(property_name, value);
    }
}

PropertyLinkTarget::~PropertyLinkTarget() = default;

std::ostream &LinkableObject::operator<<(std::ostream &out) const {
    return out;
}
std::ostream &operator<<(std::ostream &out, const LinkableObject &m) {
    return m.operator<<(out);
}

void LinkableObject::update(const Value &v) {
  if (target) {
      target->update(v);
  }
}

LinkableObject::~LinkableObject() {
    nanogui::Widget *w = dynamic_cast<nanogui::Widget *>(widget);
    if (w) w->decRef();
    if (target) delete target;
}

LinkableObject::LinkableObject() : widget(nullptr), target(nullptr) {
}

LinkableObject::LinkableObject(EditorObject *ew) : widget(ew), target(nullptr) {
    nanogui::Widget *w = dynamic_cast<nanogui::Widget *>(widget);
    if (w) w->incRef();
}

LinkableObject::LinkableObject(LinkTarget *t) : widget(nullptr), target(t) {
}

EditorObject *LinkableObject::linked() {
    if (widget) { return widget; }
    if (target) {
        PropertyLinkTarget *plt = dynamic_cast<PropertyLinkTarget*>(target);
        if (plt) { return plt->widget(); }
    }
    return nullptr;
}

void LinkableObject::unlink(const std::string & class_name, EditorWidget * widget) {
    auto link = LinkManager::instance().remote_links(class_name, widget->getName());
    if (link) {
        auto property_id_to_name = widget->reverse_property_map();
        for (auto & link_info : *link) {
            auto linkable_property = EDITOR->gui()->findLinkableProperty(link_info.remote_name);
            if (linkable_property) {
                linkable_property->unlink(widget);
            }
        }
    }

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

