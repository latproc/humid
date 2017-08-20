//
//  EditorTextBox.cpp
//  Project: humid
//
//	All rights reserved. Use of this source code is governed by the
//	3-clause BSD License in LICENSE.txt.

#include <iostream>
#include "editortextbox.h"

EditorTextBox::EditorTextBox(NamedObject *owner, Widget *parent, const std::string nam, LinkableProperty *lp, int icon)
    : TextBox(parent), EditorWidget(owner, "TEXT", nam, this, lp), dh(0), handles(9), handle_coordinates(9,2),
      valign(0), wrap_text(false) {
}

bool EditorTextBox::mouseButtonEvent(const nanogui::Vector2i &p, int button, bool down, int modifiers) {

    using namespace nanogui;

    if (editorMouseButtonEvent(this, p, button, down, modifiers))
        return nanogui::TextBox::mouseButtonEvent(p, button, down, modifiers);

    return true;
}

bool EditorTextBox::mouseMotionEvent(const nanogui::Vector2i &p, const nanogui::Vector2i &rel, int button, int modifiers) {

    if (editorMouseMotionEvent(this, p, rel, button, modifiers))
        return TextBox::mouseMotionEvent(p, rel, button, modifiers);

    return true;
}

bool EditorTextBox::mouseEnterEvent(const Vector2i &p, bool enter) {

    if (editorMouseEnterEvent(this, p, enter))
        return TextBox::mouseEnterEvent(p, enter);

    return true;
}

void EditorTextBox::getPropertyNames(std::list<std::string> &names) {
	EditorWidget::getPropertyNames(names);
	names.push_back("Number format");
  names.push_back("Text");
  names.push_back("Font Size");
  names.push_back("Alignment");
  names.push_back("Vertical Alignment");
  names.push_back("Wrap Text");
}

void EditorTextBox::loadPropertyToStructureMap(std::map<std::string, std::string> &property_map) {
  EditorWidget::loadPropertyToStructureMap(property_map);
  property_map["Text"] = "text";
  property_map["Font Size"] = "font_size";
  property_map["Alignment"] = "alignment";
  property_map["Vertical Alignment"] = "valign";
  property_map["Wrap Text"] = "wrap";
}

Value EditorTextBox::getPropertyValue(const std::string &prop) {
  Value res = EditorWidget::getPropertyValue(prop);
  if (res != SymbolTable::Null)
    return res;
  if (prop == "Text") {
    return Value(value(), Value::t_string);
  }
  if (prop == "Font Size") return fontSize();
  if (prop == "Alignment") return (int)alignment();
  if (prop == "Vertical Alignment") return valign;
  if (prop == "Wrap Text") return wrap_text ? 1 : 0;
  return SymbolTable::Null;
}


void EditorTextBox::setProperty(const std::string &prop, const std::string value) {
  EditorWidget::setProperty(prop, value);
  if (prop == "Remote") {
    if (remote) {
        remote->link(new LinkableText(this));  }
    }
    if (prop == "Text") {
      setValue(value);
    }
    if (prop == "Font Size") {
      int fs = std::atoi(value.c_str());
      setFontSize(fs);
    }
    if (prop == "Alignment") {
      setAlignment((Alignment)std::atoi(value.c_str()));
    }
    if (prop == "Vertical Alignment") valign = std::atoi(value.c_str());
    if (prop == "Wrap Text") {
      wrap_text = (value == "1" || value == "true" || value == "TRUE");
    }
}
