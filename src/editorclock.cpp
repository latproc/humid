//
//  EditorClock.cpp
//  Project: humid
//
//	All rights reserved. Use of this source code is governed by the
//	3-clause BSD License in LICENSE.txt.

#include <iostream>
#include <ctime>
#include <nanogui/widget.h>
#include <nanogui/theme.h>
#include <nanogui/opengl.h>
#include "editorwidget.h"
#include "editorclock.h"
#include "editor.h"
#include "propertyformhelper.h"
#include "helper.h"
#include "colourhelper.h"
#include <cassert>

const std::map<std::string, std::string> & EditorClock::property_map() const {
  auto structure_class = findClass("TIME");
  assert(structure_class);
  return structure_class->property_map();
}

const std::map<std::string, std::string> & EditorClock::reverse_property_map() const {
  auto structure_class = findClass("TIME");
  assert(structure_class);
  return structure_class->reverse_property_map();
}

EditorClock::EditorClock(NamedObject *owner, Widget *parent, const std::string nam,
            LinkableProperty *lp)
: Label(parent, ""), EditorWidget(owner, "TIME", nam, this, lp), mBackgroundColor(nanogui::Color(0,0)), mTextColor(nanogui::Color(0,0)),
  alignment(1), valign(1), time_format("%H:%M:%S") {
}

static std::string localTimeString(const std::string &format) {
    time_t now = time(nullptr);
    struct tm tmv;
#if defined(_WIN32)
    localtime_s(&tmv, &now);
#else
    localtime_r(&now, &tmv);
#endif
    const char *fmt = format.empty() ? "%H:%M:%S" : format.c_str();
    char buf[256];
    if (std::strftime(buf, sizeof(buf), fmt, &tmv) == 0)
        buf[0] = 0;
    return std::string(buf);
}

void EditorClock::draw(NVGcontext *ctx) {
    Widget::draw(ctx);

    // Refresh the caption from the local wall clock on every draw so the
    // widget stays current without any Clockwork update.
    setCaption(localTimeString(time_format));

    NVGcolor textColor = mTextColor.w() == 0 ? mColor : mTextColor;

    if (mBackgroundColor != nanogui::Color(0,0)) {
        nvgBeginPath(ctx);
        if (border == 0)
            nvgRect(ctx, mPos.x() + 1, mPos.y() + 1.0f, mSize.x() - 2, mSize.y() - 2);
        else {
            int a = border / 2+1;
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
    int padding = 1;
    int pos_h = mPos.x() + padding;
    if (alignment == 1) {
        align = NVG_ALIGN_CENTER;
        pos_h += mSize.x() / 2;
    }
    else if (alignment == 2) {
        align = NVG_ALIGN_RIGHT;
        pos_h += mSize.x() - 2 * padding;
    }

    int pos_v = mPos.y();
    if (valign == 1) {
      alignv = NVG_ALIGN_MIDDLE;
      pos_v = mPos.y() + mSize.y()/2;
    }
    else if (valign == 2) {
      alignv = NVG_ALIGN_BOTTOM;
      pos_v = mPos.y() + mSize.y();
    }

    if (mFixedSize.x() > 0) {
        nvgTextAlign(ctx, align | alignv);
        nvgTextBox(ctx, mPos.x(), pos_v, mFixedSize.x(), mCaption.c_str(), nullptr);
    } else {
        nvgTextAlign(ctx, align | alignv);
        nvgText(ctx, pos_h , pos_v, mCaption.c_str(), nullptr);
    }
    if (mSelected)
      drawSelectionBorder(ctx, mPos, mSize);
    else if (EDITOR->isEditMode()) {
      drawElementBorder(ctx, mPos, mSize);
    }
}

void EditorClock::loadPropertyToStructureMap(std::map<std::string, std::string> &properties) {
  properties = property_map();
}

void EditorClock::getPropertyNames(std::list<std::string> &names) {
  EditorWidget::getPropertyNames(names);
  names.push_back("Font Size");
  names.push_back("Format");
  names.push_back("Text Colour");
  names.push_back("Vertical Alignment");
  names.push_back("Alignment");
  names.push_back("Background Colour");
}

Value EditorClock::getPropertyValue(const std::string &prop) {
  Value res = EditorWidget::getPropertyValue(prop);
  if (res != SymbolTable::Null)
    return res;
  if (prop == "Format") return Value(time_format, Value::t_string);
  if (prop == "Font Size") return fontSize();
  if (prop == "Text Colour") {
    return Value(stringFromColour(mTextColor), Value::t_string);
  }
  if (prop == "Alignment") return alignment;
  if (prop == "Vertical Alignment") return valign;
  if (prop == "Background Colour" && backgroundColor() != mTheme->mTransparent) {
    return Value(stringFromColour(backgroundColor()), Value::t_string);
  }

  return SymbolTable::Null;
}

void EditorClock::setProperty(const std::string &prop, const std::string value) {
  EditorWidget::setProperty(prop, value);
  if (prop == "Format") {
    setFormat(value);
    if (getDefinition()) getDefinition()->getProperties().add("format", value);
  }
  if (prop == "Font Size") {
    int fs = std::atoi(value.c_str());
    setFontSize(fs);
  }
  if (prop == "Alignment") {
      alignment = std::atoi(value.c_str());
  }
  if (prop == "Vertical Alignment") valign = std::atoi(value.c_str());
  if (prop == "Text Colour") {
    if (getDefinition()) getDefinition()->getProperties().add("text_colour", value);
    setTextColor(colourFromProperty(getDefinition(), "text_colour"));
  }
  if (prop == "Background Colour") {
    if (getDefinition()) getDefinition()->getProperties().add("bg_color", value);
    setBackgroundColor(colourFromProperty(getDefinition(), "bg_color"));
  }
}


void EditorClock::loadProperties(PropertyFormHelper* properties) {
  EditorWidget::loadProperties(properties);
  EditorClock *clk = dynamic_cast<EditorClock*>(this);
  nanogui::Widget *w = dynamic_cast<nanogui::Widget*>(this);
  if (w) {
    properties->addVariable<std::string> (
      "Format",
      [&](std::string value) mutable{ setFormat(value); },
      [&]()->std::string{ return time_format; });
    properties->addVariable<int> (
      "Alignment",
      [&](int value) mutable{ alignment = value; },
      [&]()->int{ return alignment; });
    properties->addVariable<int> (
      "Vertical Alignment",
      [&](int value) mutable{ valign = value; },
      [&]()->int{ return valign; });
    properties->addVariable<nanogui::Color> (
      "Text Colour",
      [&,clk](const nanogui::Color &value) mutable{ clk->setTextColor(value); },
      [&,clk]()->const nanogui::Color &{ return clk->textColor(); });
    properties->addVariable<nanogui::Color> (
      "Background Colour",
      [&,clk](const nanogui::Color &value) mutable{ clk->setBackgroundColor(value); },
      [&,clk]()->const nanogui::Color &{ return clk->backgroundColor(); });
    properties->addGroup("Remote");
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
