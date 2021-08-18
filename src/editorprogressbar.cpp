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

EditorProgressBar::EditorProgressBar(NamedObject *owner, Widget *parent, const std::string nam, LinkableProperty *lp)
	: ProgressBar(parent), EditorWidget(owner, "PROGRESS", nam, this, lp) {
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
    if (mSelected)
      drawSelectionBorder(ctx, mPos, mSize);
    else if (EDITOR->isEditMode()) {
      drawElementBorder(ctx, mPos, mSize);
    }
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

void EditorProgressBar::setProperty(const std::string &prop, const std::string value) {
  EditorWidget::setProperty(prop, value);
  if (prop == "Remote") {
    if (remote) {
        remote->link(new LinkableNumber(this));  }
    }
}


void EditorProgressBar::loadProperties(PropertyFormHelper* properties) {
  EditorWidget::loadProperties(properties);
  nanogui::Widget *w = dynamic_cast<nanogui::Widget*>(this);
  if (w) {
    properties->addVariable<float> (
      "Value",
      [&](float value) mutable { setValue(value); },
      [&]()->float{ return value(); });
    properties->addGroup("Remote");
    properties->addVariable<std::string> (
      "Remote object",
      [&, w, this,properties](std::string value) mutable {
        LinkableProperty *lp = EDITOR->gui()->findLinkableProperty(value);
        this->setRemoteName(value);
        if (remote) remote->unlink(this);
        remote = lp;
        if (lp) { lp->link(new LinkableNumber(this)); }
        //properties->refresh();
       },
      [&]()->std::string{
        if (remote) return remote->tagName();
        if (getDefinition()) {
          const Value &rmt_v = getDefinition()->getProperties().find("remote");
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
