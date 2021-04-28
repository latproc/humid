//
//  EditorFrame.cpp
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
#include "editorframe.h"
#include "propertyformhelper.h"

EditorFrame::EditorFrame(NamedObject *owner, nanogui::Widget *parent, const std::string nam, LinkableProperty *lp)
	: nanogui::Widget(parent), EditorWidget(owner, "FRAME", nam, this, lp), dh(0), handles(9), handle_coordinates(9,2) {
	}

bool EditorFrame::mouseButtonEvent(const nanogui::Vector2i &p, int button, bool down, int modifiers) {
    using namespace nanogui;

    if (editorMouseButtonEvent(this, p, button, down, modifiers))
        return nanogui::Widget::mouseButtonEvent(p, button, down, modifiers);

    return true;
}

bool EditorFrame::mouseMotionEvent(const nanogui::Vector2i &p, const nanogui::Vector2i &rel, int button, int modifiers) {

    if (editorMouseMotionEvent(this, p, rel, button, modifiers))
        return nanogui::Widget::mouseMotionEvent(p, rel, button, modifiers);

    return true;
}

bool EditorFrame::mouseEnterEvent(const Vector2i &p, bool enter) {

    if (editorMouseEnterEvent(this, p, enter))
        return nanogui::Widget::mouseEnterEvent(p, enter);

    return true;
}

void EditorFrame::draw(NVGcontext *ctx) {
    nanogui::Widget::draw(ctx);
    nvgBeginPath(ctx);
    nvgStrokeWidth(ctx, 1.0);
    nvgMoveTo(ctx, mPos.x(), mPos.y() + mSize.y());
    nvgLineTo(ctx, mPos.x() + mSize.x(), mPos.y() + mSize.y());
    nvgLineTo(ctx, mPos.x() + mSize.x(), mPos.y());
    nvgLineTo(ctx, mPos.x(), mPos.y());
    if (mSelected)
      drawSelectionBorder(ctx, mPos, mSize);
    else if (EDITOR->isEditMode()) {
      drawElementBorder(ctx, mPos, mSize);
    }
}

void EditorFrame::loadPropertyToStructureMap(std::map<std::string, std::string> &property_map) {
	EditorWidget::loadPropertyToStructureMap(property_map);
}

void EditorFrame::getPropertyNames(std::list<std::string> &names) {
    EditorWidget::getPropertyNames(names);
}

Value EditorFrame::getPropertyValue(const std::string &prop) {
	Value res = EditorWidget::getPropertyValue(prop);
  if (res != SymbolTable::Null)
    return res;
  return SymbolTable::Null;
}

void EditorFrame::setProperty(const std::string &prop, const std::string value) {
  EditorWidget::setProperty(prop, value);
  if (prop == "Remote") {
    if (remote) {
        remote->link(new LinkableNumber(this));  }
    }
}


void EditorFrame::loadProperties(PropertyFormHelper* properties) {
  EditorWidget::loadProperties(properties);
  nanogui::Widget *w = dynamic_cast<nanogui::Widget*>(this);
  if (w) {
    properties->addVariable<float> (
      "Value",
      [&](float value) mutable {  },
      [&]()->float{ return 0.0f; });
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
