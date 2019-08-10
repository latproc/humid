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

EditorLabel::EditorLabel(NamedObject *owner, Widget *parent, const std::string nam,
            LinkableProperty *lp, const std::string caption,
            const std::string &font, int fontSize, int icon)
: Label(parent, caption), EditorWidget(owner, "LABEL", nam, this, lp), dh(0), handles(9), handle_coordinates(9,2), mBackgroundColor(nanogui::Color(0,0)), mTextColor(nanogui::Color(0,0)),
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
  EditorWidget::loadPropertyToStructureMap(property_map);
  property_map["Caption"] = "caption";
  property_map["Font Size"] = "font_size";
  property_map["Text Color"] = "text_color";
  property_map["Alignment"] = "alignment";
  property_map["Vertical Alignment"] = "valign";
  property_map["Wrap Text"] = "wrap";
  property_map["Background Colour"] = "bg_color";
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