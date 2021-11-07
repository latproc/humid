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
#include "propertyformhelper.h"
#include "helper.h"
#include "colourhelper.h"

const std::map<std::string, std::string> & EditorProgressBar::property_map() const {
  auto structure_class = findClass("PROGRESS");
  assert(structure_class);
  return structure_class->property_map();
}

const std::map<std::string, std::string> & EditorProgressBar::reverse_property_map() const {
  auto structure_class = findClass("PROGRESS");
  assert(structure_class);
  return structure_class->reverse_property_map();
}

EditorProgressBar::EditorProgressBar(NamedObject *owner, Widget *parent, const std::string nam, LinkableProperty *lp)
	: ProgressBar(parent), EditorWidget(owner, "PROGRESS", nam, this, lp),fg_color(nanogui::Color(192,192,192,255)), bg_color(nanogui::Color(240,240,240,255)) {
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
	/* the following is based on nanogui::progressbar::draw */
    Widget::draw(ctx);

    NVGpaint paint = nvgBoxGradient(
        ctx, mPos.x() + 1, mPos.y() + 1,
        mSize.x()-2, mSize.y(), 3, 4, bg_color, bg_color);
    nvgBeginPath(ctx);
    nvgRoundedRect(ctx, mPos.x(), mPos.y(), mSize.x(), mSize.y(), 3);
    nvgFillPaint(ctx, paint);
    nvgFill(ctx);

    float value = std::min(std::max(0.0f, mValue / value_scale), 1.0f);

    int barPos = (int) std::round((mSize.x() - 2) * value);

    paint = nvgBoxGradient(
        ctx, mPos.x(), mPos.y(),
        barPos+1.5f, mSize.y()-1, 3, 4,
        fg_color, fg_color);

    nvgBeginPath(ctx);
    nvgRoundedRect(
        ctx, mPos.x()+1, mPos.y()+1,
        barPos, mSize.y()-2, 3);
    nvgFillPaint(ctx, paint);
    nvgFill(ctx);
    if (mSelected)
      drawSelectionBorder(ctx, mPos, mSize);
    else if (EDITOR->isEditMode()) {
      drawElementBorder(ctx, mPos, mSize);
    }
}

void EditorProgressBar::loadPropertyToStructureMap(std::map<std::string, std::string> &properties) {
  properties = property_map();
}

void EditorProgressBar::getPropertyNames(std::list<std::string> &names) {
    EditorWidget::getPropertyNames(names);
    names.push_back("Foreground Colour");
    names.push_back("Background Colour");
}

Value EditorProgressBar::getPropertyValue(const std::string &prop) {
	Value res = EditorWidget::getPropertyValue(prop);
  if (res != SymbolTable::Null)
    return res;
  if (prop == "Foreground Colour") {
    nanogui::Widget *w = dynamic_cast<nanogui::Widget*>(this);
    nanogui::Button *btn = dynamic_cast<nanogui::Button*>(this);
    return Value(stringFromColour(color()), Value::t_string);
  }
  if (prop == "Background Colour") {
    nanogui::Widget *w = dynamic_cast<nanogui::Widget*>(this);
    nanogui::Button *btn = dynamic_cast<nanogui::Button*>(this);
    return Value(stringFromColour(backgroundColor()), Value::t_string);
  }
  return SymbolTable::Null;
}

void EditorProgressBar::setProperty(const std::string &prop, const std::string value) {
  EditorWidget::setProperty(prop, value);
  if (prop == "Remote") {
    if (remote) {
        remote->link(new LinkableNumber(this));  }
    }
  else if (prop == "Foreground Colour") {
    getDefinition()->getProperties().add("fg_color", value);
    setBackgroundColor(colourFromProperty(getDefinition(), "fg_color"));
  }
  else if (prop == "Background Colour") {
    getDefinition()->getProperties().add("bg_color", value);
    setBackgroundColor(colourFromProperty(getDefinition(), "bg_color"));
  }
}


void EditorProgressBar::loadProperties(PropertyFormHelper* properties) {
  EditorWidget::loadProperties(properties);
  nanogui::Widget *w = dynamic_cast<nanogui::Widget*>(this);
  if (w) {
    EditorGUI *gui = EDITOR->gui();
    properties->addVariable<float> (
      "Value",
      [&](float value) mutable { setValue(value); },
      [&]()->float{ return value(); });
    properties->addVariable<nanogui::Color> (
      "Foreground colour",
       [&](const nanogui::Color &value) mutable{ setColor(value); },
	   [&]()->const nanogui::Color &{ return color(); });
    properties->addVariable<nanogui::Color> (
      "Background colour",
       [&](const nanogui::Color &value) mutable{ setBackgroundColor(value); },
       [&]()->const nanogui::Color &{ return backgroundColor(); });
    properties->addGroup("Remote");
    properties->addVariable<std::string> (
      "Remote object",
      [&, w, this,properties](std::string value) mutable {
        LinkableProperty *lp = EDITOR->gui()->findLinkableProperty(value);
        this->setRemoteName(value);
        if (remote) remote->unlink(this);
        remote = lp;
        if (lp) { lp->link(new LinkableNumber(this)); }
       },
      [&]()->std::string{
        if (remote) return remote->tagName();
        if (getDefinition()) {
          const Value &rmt_v = getDefinition()->getValue("remote");
          if (rmt_v != SymbolTable::Null)
            return rmt_v.asString();
        }
        return "";
      });
    properties->addVariable<std::string> (
      "Connection",
      [&,this,properties](std::string value) {
        if (remote) remote->setGroup(value); else setConnection(value);
       },
      [&]()->std::string{ return remote ? remote->group() : getConnection(); });
    properties->addVariable<std::string> (
      "Visibility",
      [&,this,properties](std::string value) {
        LinkableProperty *lp = EDITOR->gui()->findLinkableProperty(value);
        if (visibility) visibility->unlink(this);
        visibility = lp;
        if (lp) { lp->link(new LinkableVisibility(this)); }
       },
      [&]()->std::string{ return visibility ? visibility->tagName() : ""; });
  }
}
