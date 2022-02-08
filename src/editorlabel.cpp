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
#include "colourhelper.h"
#include "valuehelper.h"

const std::map<std::string, std::string> & EditorLabel::property_map() const {
  auto structure_class = findClass("LABEL");
  assert(structure_class);
  return structure_class->property_map();
}

const std::map<std::string, std::string> & EditorLabel::reverse_property_map() const {
  auto structure_class = findClass("LABEL");
  assert(structure_class);
  return structure_class->reverse_property_map();
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
      if (border == 0)
        nvgRect(ctx, mPos.x() + 1, mPos.y() + 1.0f, mSize.x() - 2, mSize.y() - 2);
      else {
        int a = border / 2;
        nvgRoundedRect(ctx, mPos.x() + a, mPos.y() + a, mSize.x()-2*a,
                 mSize.y()-2*a, mTheme->mButtonCornerRadius);
      }
      nvgFillColor(ctx, nanogui::Color(mBackgroundColor));
      nvgFill(ctx);
    }

    if (border > 0) {
      nvgBeginPath(ctx);
      nvgStrokeWidth(ctx, border);
      int a = border / 2;
      nvgRoundedRect(ctx, mPos.x() + a, mPos.y()+a, mSize.x() - 2*a,
                    mSize.y() - 2*a, mTheme->mButtonCornerRadius);
      nvgStrokeColor(ctx, mTheme->mBorderMedium);
      nvgStroke(ctx);
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

    std::string valStr = format_caption(mCaption, format_string, value_type, value_scale);
    if (format_string == "password") {
      for (size_t i = 0; i<valStr.length(); ++i) {
        valStr[i] = '*';
      }
    }

    int pos_v = mPos.y();
    if (valign != 0) {
      float bounds[4];
      nvgTextBoxBounds(ctx, mPos.x(), mPos.y(), mSize.x(), "A", nullptr, bounds);
      float line_height = bounds[3] - bounds[1];
      if (wrap_text) {
        nvgTextBoxBounds(ctx, mPos.x(), mPos.y(), mSize.x(), valStr.c_str(), nullptr, bounds);
      }
      if (valign == 1) {
        alignv = NVG_ALIGN_MIDDLE;
        pos_v = mPos.y() + mSize.y()/2 - (bounds[3] - bounds[1] - line_height) / 2.0;
      }
      else if (valign == 2) {
        alignv = NVG_ALIGN_BOTTOM;
        pos_v = mPos.y() + mSize.y() - (bounds[3] - bounds[1] - line_height);
      }
    }

    if (mFixedSize.x() > 0) {
        nvgTextAlign(ctx, align | alignv);
        if (wrap_text)
          nvgTextBox(ctx, mPos.x(), pos_v, mFixedSize.x(), valStr.c_str(), nullptr);
        else {
          if (align == NVG_ALIGN_LEFT)
            nvgText(ctx, mPos.x(), pos_v, valStr.c_str(), nullptr);
          else if (align == NVG_ALIGN_CENTER)
            nvgText(ctx, mPos.x() + mSize.x()/2, pos_v, valStr.c_str(), nullptr);
          else if (align == NVG_ALIGN_RIGHT)
            nvgText(ctx, mPos.x() + mSize.x(), pos_v, valStr.c_str(), nullptr);
        }
    } else {
        nvgTextAlign(ctx, align | alignv);
        nvgText(ctx, mPos.x(), pos_v, valStr.c_str(), nullptr);
    }
    if (mSelected)
      drawSelectionBorder(ctx, mPos, mSize);
    else if (EDITOR->isEditMode()) {
      drawElementBorder(ctx, mPos, mSize);
    }
}

void EditorLabel::loadPropertyToStructureMap(std::map<std::string, std::string> &properties) {
  properties = property_map();
}

void EditorLabel::getPropertyNames(std::list<std::string> &names) {
  EditorWidget::getPropertyNames(names);
  names.push_back("Caption");
  names.push_back("Font Size");
  names.push_back("Text Colour");
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
  if (prop == "Text Colour") {
      nanogui::Widget *w = dynamic_cast<nanogui::Widget*>(this);
      nanogui::Label *lbl = dynamic_cast<nanogui::Label*>(this);
    return Value(stringFromColour(mTextColor), Value::t_string);
  }
  if (prop == "Alignment") return alignment;
  if (prop == "Vertical Alignment") return valign;
  if (prop == "Wrap Text") return wrap_text ? 1 : 0;
  if (prop == "Background Colour" && backgroundColor() != mTheme->mTransparent) {
    return Value(stringFromColour(backgroundColor()), Value::t_string);
  }

  return SymbolTable::Null;
}

void EditorLabel::setProperty(const std::string &prop, const std::string value) {
  EditorWidget::setProperty(prop, value);
  if (prop == "Caption") {
    setCaption(value);
  }
  if (prop == "Remote") {
    if (remote) {
      remote->link(new LinkableText(this));
    }
  }
  if (prop == "Font Size") {
    int fs = std::atoi(value.c_str());
    setFontSize(fs);
  }
  if (prop == "Alignment") {
    long align_int = 0;
    Value val(value);
    if (val.asInteger(align_int)) {
      alignment = static_cast<int>(align_int);
    }
    else {
      if (value == "left") { alignment = 0;}
      else if (value == "centre" || value == "center") { alignment = 1; }
      else if (value == "right") { alignment = 2; }
      else alignment = defaultForProperty("alignment").iValue;
    }
  }
  if (prop == "Vertical Alignment") {
    long v_align_int = 0;
    Value val(value);
    if (val.asInteger(v_align_int)) {
      valign = static_cast<int>(v_align_int);
    }
    else {
      if (value == "top") { valign = 0;}
      else if (value == "centre" || value == "center") { valign = 1; }
      else if (value == "bottom") { valign = 2; }
      else valign = defaultForProperty("valign").iValue;
    }
  }
  if (prop == "Wrap Text") {
    wrap_text = (value == "1" || value == "true" || value == "TRUE");
  }
  if (prop == "Text Colour") {
    getDefinition()->getProperties().add("text_colour", value);
    setTextColor(colourFromProperty(getDefinition(), "text_colour"));
  }
  if (prop == "Background Colour") {
    getDefinition()->getProperties().add("bg_color", value);
    setBackgroundColor(colourFromProperty(getDefinition(), "bg_color"));
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
