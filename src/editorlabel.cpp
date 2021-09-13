//
//  EditorLabel.cpp
//  Project: humid
//
//	All rights reserved. Use of this source code is governed by the
//	3-clause BSD License in LICENSE.txt.

#include <iostream>
#include <nanogui/widget.h>
#include <nanogui/theme.h>
#include <nanogui/opengl.h>
#include <nanogui/widget.h>
#include "editorwidget.h"
#include "editorlabel.h"
#include "editor.h"
#include "propertyformhelper.h"
#include "helper.h"

static std::map<std::string, std::string> standard_property_map; // friendly name -> symbol
static std::map<std::string, std::string> inverted_property_map; // symbol -> friendly name

std::map<std::string, std::string> *EditorLabel::property_map() {
  if (standard_property_map.empty()) { loadPropertyToStructureMap(standard_property_map); }
  return &standard_property_map;
}

std::map<std::string, std::string> *EditorLabel::reverse_property_map() {
  if (inverted_property_map.empty()) {
    invert_map(*property_map(), inverted_property_map);
  }
  return &inverted_property_map;
}

EditorLabel::EditorLabel(NamedObject *owner, Widget *parent, const std::string nam,
            LinkableProperty *lp, const std::string caption,
            const std::string &font, int fontSize, int icon)
: Label(parent, caption), EditorWidget(owner, "LABEL", nam, this, lp), mBackgroundColor(nanogui::Color(0,0)), mTextColor(nanogui::Color(0,0)),
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
    Widget::draw(ctx);
    NVGcolor textColor = mTextColor.w() == 0 ? mColor : mTextColor;

  if (mBackgroundColor != nanogui::Color(0,0)) {
      nvgBeginPath(ctx);
      nvgRect(ctx, mPos.x() + 1, mPos.y() + 1.0f, mSize.x() - 2, mSize.y() - 2);
      nvgFillColor(ctx, nanogui::Color(mBackgroundColor));
      nvgFill(ctx);
    }

    nvgFontFace(ctx, mFont.c_str());
    nvgFontSize(ctx, fontSize());
    nvgFillColor(ctx, textColor);
    int align = NVG_ALIGN_LEFT;
    int alignv = NVG_ALIGN_TOP;
    if (alignment == 1)
      align = NVG_ALIGN_CENTER;
    else if (alignment == 2)
      align = NVG_ALIGN_RIGHT;

    int pos_v = mPos.y();
    if (valign == 1) {
      alignv = NVG_ALIGN_MIDDLE;
      pos_v = mPos.y() + mSize.y()/2;
    }
    else if (valign == 2) {
      alignv = NVG_ALIGN_BOTTOM;
      pos_v = mPos.y() + mSize.y();
    }

    std::string valStr(mCaption);
    float scale = value_scale;
    if (scale == 0.0f) scale = 1.0f;
    if (format_string.length()) {
      if (value_type == Value::t_integer) {// integer
        char buf[20];
        long val = std::atol(valStr.c_str());
        snprintf(buf, 20, format_string.c_str(), (long)(val / scale));
        valStr = buf;
      }
      else if (value_type == Value::t_float) {
        char buf[20];
        float val = std::atof(valStr.c_str());
        snprintf(buf, 20, format_string.c_str(), val / scale);
        valStr = buf;       
      }
    } 
    else if (value_type == Value::t_float) {
        char buf[20];
        float val = std::atof(valStr.c_str());
        snprintf(buf, 20, "%5.3f", val / scale);
        valStr = buf;       
    }
    else if (value_type == Value::t_integer) {
        char buf[20];
        long val = std::atol(valStr.c_str());
        snprintf(buf, 20, "%ld", (long)(val / scale));
        valStr = buf;
    }

    if (mFixedSize.x() > 0) {
        nvgTextAlign(ctx, align | alignv);
        nvgTextBox(ctx, mPos.x(), pos_v, mFixedSize.x(), valStr.c_str(), nullptr);
    } else {
        nvgTextAlign(ctx, align | alignv);
        nvgText(ctx, mPos.x(), mPos.y() + mSize.y() * 0.5f, valStr.c_str(), nullptr);
    }
    if (mSelected)
      drawSelectionBorder(ctx, mPos, mSize);
    else if (EDITOR->isEditMode()) {
      drawElementBorder(ctx, mPos, mSize);
    }
}

void EditorLabel::loadPropertyToStructureMap(std::map<std::string, std::string> &property_map) {
  if (standard_property_map.empty()) {
    EditorWidget::loadPropertyToStructureMap(standard_property_map);
    standard_property_map["Caption"] = "caption";
    standard_property_map["Font Size"] = "font_size";
    standard_property_map["Text Color"] = "text_color";
    standard_property_map["Alignment"] = "alignment";
    standard_property_map["Vertical Alignment"] = "valign";
    standard_property_map["Wrap Text"] = "wrap";
    standard_property_map["Background Colour"] = "bg_color";
  }
  property_map = standard_property_map;
}

void EditorLabel::getPropertyNames(std::list<std::string> &names) {
  EditorWidget::getPropertyNames(names);
  names.push_back("Caption");
  names.push_back("Font Size");
  names.push_back("Text Color");
  names.push_back("Vertical Alignment");
  names.push_back("Alignment");
  names.push_back("Wrap Text");
  names.push_back("Background Colour");
}

Value EditorLabel::getPropertyValue(const std::string &prop) {
  Value res = EditorWidget::getPropertyValue(prop);
  if (res != SymbolTable::Null)
    return res;
  if (prop == "Caption") return Value(caption(), Value::t_string);
  if (prop == "Font Size") return fontSize();
  if (prop == "Text Color") {
      nanogui::Widget *w = dynamic_cast<nanogui::Widget*>(this);
      nanogui::Label *lbl = dynamic_cast<nanogui::Label*>(this);
      char buf[50];
      snprintf(buf, 50, "%5.4f,%5.4f,%5.4f,%5.4f", mTextColor.r(), mTextColor.g(), mTextColor.b(), mTextColor.w());
      return Value(buf, Value::t_string);
  }
  if (prop == "Alignment") return alignment;
  if (prop == "Vertical Alignment") return valign;
  if (prop == "Wrap Text") return wrap_text ? 1 : 0;
  if (prop == "Background Colour" && backgroundColor() != mTheme->mTransparent) {
    char buf[50];
    snprintf(buf, 50, "%5.4f,%5.4f,%5.4f,%5.4f",
             backgroundColor().r(), backgroundColor().g(), backgroundColor().b(), backgroundColor().w());
    return Value(buf, Value::t_string);
  }

  return SymbolTable::Null;
}

void EditorLabel::setProperty(const std::string &prop, const std::string value) {
  EditorWidget::setProperty(prop, value);
    if (prop == "Remote") {
      if (remote) {
        remote->link(new LinkableText(this));
      }
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


void EditorLabel::loadProperties(PropertyFormHelper* properties) {
  EditorWidget::loadProperties(properties);
  EditorLabel *lbl = dynamic_cast<EditorLabel*>(this);
  nanogui::Widget *w = dynamic_cast<nanogui::Widget*>(this);
  if (w) {
    properties->addVariable<std::string> (
      "Caption",
      [&](std::string value) mutable{ setCaption(value); },
      [&]()->std::string{ return caption(); });
    properties->addVariable<int> (
      "Alignment",
      [&](int value) mutable{ alignment = value; },
      [&]()->int{ return alignment; });
    properties->addVariable<int> (
      "Vertical Alignment",
      [&](int value) mutable{ valign = value; },
      [&]()->int{ return valign; });
    properties->addVariable<bool> (
      "Wrap Text",
      [&](bool value) mutable{ wrap_text = value; },
      [&]()->bool{ return wrap_text; });
    properties->addVariable<nanogui::Color> (
      "Text Colour",
      [&,lbl](const nanogui::Color &value) mutable{ lbl->setTextColor(value); },
      [&,lbl]()->const nanogui::Color &{ return lbl->textColor(); });
    properties->addVariable<nanogui::Color> (
      "Background Colour",
      [&,lbl](const nanogui::Color &value) mutable{ lbl->setBackgroundColor(value); },
      [&,lbl]()->const nanogui::Color &{ return lbl->backgroundColor(); });
    properties->addGroup("Remote");
    properties->addVariable<std::string> (
      "Remote object",
      [&,this,properties](std::string value) {
        LinkableProperty *lp = EDITOR->gui()->findLinkableProperty(value);
        this->setRemoteName(value);
        if (remote) remote->unlink(this);
        remote = lp;
        if (lp) { lp->link(new LinkableText(this)); }
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
