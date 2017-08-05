//
//  EditorButton.cpp
//  Project: humid
//
//	All rights reserved. Use of this source code is governed by the
//	3-clause BSD License in LICENSE.txt.

#include <iostream>
#include "editorwidget.h"
#include "editorbutton.h"

std::string stripEscapes(const std::string &s);

EditorButton::EditorButton(NamedObject *owner, Widget *parent, const std::string &btn_name, LinkableProperty *lp, const std::string &caption,
            bool toggle, int icon)
	: Button(parent, caption, icon), EditorWidget(owner, "BUTTON", btn_name, this, lp), is_toggle(toggle){
}

bool EditorButton::mouseButtonEvent(const nanogui::Vector2i &p, int button, bool down, int modifiers) {

    using namespace nanogui;
    if (editorMouseButtonEvent(this, p, button, down, modifiers)) {
        //if (!is_toggle || (is_toggle && down))
          return nanogui::Button::mouseButtonEvent(p, button, down, modifiers);
    }

    return true;
}

bool EditorButton::mouseMotionEvent(const nanogui::Vector2i &p, const nanogui::Vector2i &rel, int button, int modifiers) {

    if (editorMouseMotionEvent(this, p, rel, button, modifiers))
        return Button::mouseMotionEvent(p, rel, button, modifiers);

    return true;
}

bool EditorButton::mouseEnterEvent(const Vector2i &p, bool enter) {
    if (editorMouseEnterEvent(this, p, enter))
        return Button::mouseEnterEvent(p, enter);

    return true;
}

void EditorButton::setCommand(std::string cmd) {
    command_str = stripEscapes(cmd);
}

const std::string &EditorButton::command() const { return command_str; }

void EditorButton::draw(NVGcontext *ctx) {
    nanogui::Button::draw(ctx);
    if (mSelected) drawSelectionBorder(ctx, mPos, mSize);
}
void EditorButton::getPropertyNames(std::list<std::string> &names) {
    EditorWidget::getPropertyNames(names);
		names.push_back("Caption");
    names.push_back("Background colour");
    names.push_back("Text colour");
    names.push_back("Behaviour");
    names.push_back("Command");
}

void EditorButton::setProperty(const std::string &prop, const std::string value) {
  EditorWidget::setProperty(prop, value);
  if (prop == "Remote") {
    if (remote) {
      if (getDefinition()->getKind() == "INDICATOR")
        remote->link(new LinkableIndicator(this));  }
    }
}


void EditorButton::loadPropertyToStructureMap(std::map<std::string, std::string> &property_map) {
  EditorWidget::loadPropertyToStructureMap(property_map);
  property_map["Caption"] = "caption";
  property_map["Background colour"] = "bg_color";
  property_map["Text colour"] = "text_colour";
  property_map["Behaviour"] = "behaviour";
  property_map["Command"] = "command";
}

Value EditorButton::getPropertyValue(const std::string &prop) {
  Value res = EditorWidget::getPropertyValue(prop);
  if (res != SymbolTable::Null)
    return res;

  if (prop == "Caption")
    return Value(caption(), Value::t_string);
  if (prop == "Background colour") {
    nanogui::Widget *w = dynamic_cast<nanogui::Widget*>(this);
    nanogui::Button *btn = dynamic_cast<nanogui::Button*>(this);
    char buf[50];
    snprintf(buf, 50, "%5.4f,%5.4f,%5.4f,%5.4f",
      backgroundColor().r(), backgroundColor().g(), backgroundColor().b(), backgroundColor().w());
    return Value(buf, Value::t_string);
  }
  if (prop == "Text colour") {
    nanogui::Widget *w = dynamic_cast<nanogui::Widget*>(this);
    char buf[50];
    snprintf(buf, 50, "%5.4f,%5.4f,%5.4f,%5.4f", mTextColor.r(), mTextColor.g(), mTextColor.b(), mTextColor.w());
    return Value(buf, Value::t_string);
  }
  if (prop == "Command" && command().length()) {
    return Value(command(), Value::t_string);
  }
  if (prop == "Behaviour") {
    return flags();
  }
  return SymbolTable::Null;
}
