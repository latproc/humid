//
//  EditorLabel.cpp
//  Project: humid
//
//	All rights reserved. Use of this source code is governed by the
//	3-clause BSD License in LICENSE.txt.

#include <iostream>
#include "editorwidget.h"
#include "editorlabel.h"

EditorLabel::EditorLabel(NamedObject *owner, Widget *parent, const std::string nam,
            LinkableProperty *lp, const std::string caption,
            const std::string &font, int fontSize, int icon)
: Label(parent, caption), EditorWidget(owner, "LABEL", nam, this, lp), dh(0), handles(9), handle_coordinates(9,2),
  alignment(1), valign(1), wrap_text(true) {
}

bool EditorLabel::mouseButtonEvent(const nanogui::Vector2i &p, int button, bool down, int modifiers) {

    using namespace nanogui;

    if (editorMouseButtonEvent(this, p, button, down, modifiers))
        return nanogui::Label::mouseButtonEvent(p, button, down, modifiers);

    return true;
}

bool EditorLabel::mouseMotionEvent(const nanogui::Vector2i &p, const nanogui::Vector2i &rel, int button, int modifiers) {

    if (editorMouseMotionEvent(this, p, rel, button, modifiers))
        return Label::mouseMotionEvent(p, rel, button, modifiers);

    return true;
}

bool EditorLabel::mouseEnterEvent(const Vector2i &p, bool enter) {

    if (editorMouseEnterEvent(this, p, enter))
        return Label::mouseEnterEvent(p, enter);

    return true;
}

void EditorLabel::draw(NVGcontext *ctx) {
    nanogui::Label::draw(ctx);
    if (mSelected) drawSelectionBorder(ctx, mPos, mSize);
}

void EditorLabel::loadPropertyToStructureMap(std::map<std::string, std::string> &property_map) {
  EditorWidget::loadPropertyToStructureMap(property_map);
  property_map["Caption"] = "caption";
  property_map["Font Size"] = "font_size";
  property_map["Alignment"] = "alignment";
  property_map["Vertical Alignment"] = "valign";
  property_map["Wrap Text"] = "wrap";
}

void EditorLabel::getPropertyNames(std::list<std::string> &names) {
    EditorWidget::getPropertyNames(names);
    names.push_back("Caption");
    names.push_back("Font Size");
    names.push_back("Vertical Alignment");
    names.push_back("Alignment");
    names.push_back("Wrap Text");
}

Value EditorLabel::getPropertyValue(const std::string &prop) {
  Value res = EditorWidget::getPropertyValue(prop);
  if (res != SymbolTable::Null)
    return res;
  if (prop == "Caption") return Value(caption(), Value::t_string);
  if (prop == "Font Size") return fontSize();
  if (prop == "Alignment") return alignment;
  if (prop == "Vertical Alignment") return valign;
  if (prop == "Wrap Text") return wrap_text ? 1 : 0;
  return SymbolTable::Null;
}

void EditorLabel::setProperty(const std::string &prop, const std::string value) {
  EditorWidget::setProperty(prop, value);
  if (prop == "Remote") {
    if (remote) {
        remote->link(new LinkableText(this));  }
    }
    if (prop == "Font Size") {
      int fs = std::atoi(value.c_str());
      setFontSize(fs);
    }
    if (prop == "Alignment") alignment = std::atoi(value.c_str());
    if (prop == "Vertical Alignment") valign = std::atoi(value.c_str());
    if (prop == "Wrap Text") {
      wrap_text = (value == "1" || value == "true" || value == "TRUE");
    }

}
