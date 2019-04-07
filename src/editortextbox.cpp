//
//  EditorTextBox.cpp
//  Project: humid
//
//	All rights reserved. Use of this source code is governed by the
//	3-clause BSD License in LICENSE.txt.

#include <iostream>
#include <nanogui/theme.h>
#include <nanogui/opengl.h>
#include <nanogui/widget.h>
#include <nanogui/entypo.h>
#include "editor.h"

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
  if (prop == "Text")
    return Value(value(), Value::t_string);
  else if (prop == "Font Size") return fontSize();
  else if (prop == "Alignment") return (int)alignment();
  else if (prop == "Vertical Alignment") return valign;
  else if (prop == "Wrap Text") return wrap_text ? 1 : 0;
  return SymbolTable::Null;
}

std::string EditorTextBox::getScaledValue(bool scaleUp) {
  if (value_scale != 1.0f && (value_type == Value::t_integer || value_type == Value::t_float) ) {
    const char *p = value().c_str();
    while (*p && (!isdigit(*p) || *p=='0')) ++p;
    if (value_type == Value::t_integer && value_scale != 1.0f) {
      long i_value = std::atol(p) * ((scaleUp) ? value_scale : 1.0f / value_scale);
      char buf[20];
      if (format_string.empty())
        snprintf(buf, 20, "%ld", i_value);
      else
        snprintf(buf, 20, format_string.c_str(), i_value);
      return buf;
    }
    else if (value_type == Value::t_float) {
      std::string v;
      double f_value = std::atof(p) * ((scaleUp) ? value_scale : 1.0f / value_scale);
      char buf[20];
      if (format_string.empty())
        snprintf(buf, 20, "%5.3lf", f_value);
      else
        snprintf(buf, 20, format_string.c_str(), f_value);
      return buf;
    }
  }
  return value();
}
/*
int EditorTextBox::getScaledInteger(bool scaleUp) {
  if (value_type == Value::t_float) {
    const char *p = value().c_str();
    while (*p && (!isdigit(*p) || *p=='0')) ++p;
    float f_value = std::atof(p);
    if (value_scale != 1.0f) f_value *= (scaleUp) ? value_scale : 1.0f / value_scale;
    return f_value;
  }
  const char *p = value().c_str();
  while (*p && (!isdigit(*p) || *p=='0')) ++p;
  int i_value = std::atoi(p);
  if (value_scale != 1.0f) i_value *= (scaleUp) ? value_scale : 1.0f / value_scale;
  return i_value;
}

float EditorTextBox::getScaledFloat(bool scaleUp) {
  if (value_type == Value::t_integer) {
    const char *p = value().c_str();
    while (*p && (!isdigit(*p) || *p=='0')) ++p;
    int i_value = std::atoi(p);
    if (value_scale != 1.0f) i_value *= (scaleUp) ? value_scale : 1.0f / value_scale;
    return i_value;
  }
  const char *p = value().c_str();
  while (*p && (!isdigit(*p) || *p=='0')) ++p;
  float f_value = std::atof(p);
  if (value_scale != 1.0f) f_value *= (scaleUp) ? value_scale : 1.0f / value_scale;
  return f_value;
}
*/

void EditorTextBox::setProperty(const std::string &prop, const std::string value) {
  EditorWidget::setProperty(prop, value);
  if (prop == "Remote") {
    if (remote) {
        remote->link(new LinkableText(this));  }
    }
    if (prop == "Text") {
      setValue(value);
    }
    else if (prop == "Font Size") {
      int fs = std::atoi(value.c_str());
      setFontSize(fs);
    }
    else if (prop == "Alignment") {
      setAlignment((Alignment)std::atoi(value.c_str()));
    }
    else if (prop == "Vertical Alignment") valign = std::atoi(value.c_str());
    else if (prop == "Wrap Text") {
      wrap_text = (value == "1" || value == "true" || value == "TRUE");
    }
}


bool EditorTextBox::focusEvent(bool focused) {
    bool res = TextBox::focusEvent(focused);
    if (!res) return res;
    if (value_scale == 1.0f) return res;

    if (mEditable) {
        if (focused)
            mValueTemp = getScaledValue(false);
        else
            mValue =  mValueTemp; mValue = getScaledValue(true);
    }

    return true;
}

void EditorTextBox::draw(NVGcontext* ctx) {
  using namespace nanogui;

    Widget::draw(ctx);

    NVGpaint bg = nvgBoxGradient(ctx,
        mPos.x() + 1, mPos.y() + 1 + 1.0f, mSize.x() - 2, mSize.y() - 2,
        3, 4, Color(255, 32), Color(32, 32));
    NVGpaint fg1 = nvgBoxGradient(ctx,
        mPos.x() + 1, mPos.y() + 1 + 1.0f, mSize.x() - 2, mSize.y() - 2,
        3, 4, Color(150, 32), Color(32, 32));
    NVGpaint fg2 = nvgBoxGradient(ctx,
        mPos.x() + 1, mPos.y() + 1 + 1.0f, mSize.x() - 2, mSize.y() - 2,
        3, 4, nvgRGBA(255, 0, 0, 100), nvgRGBA(255, 0, 0, 50));

    nvgBeginPath(ctx);
    if (border)
      nvgRoundedRect(ctx, mPos.x() + border, mPos.y() + border + 1.0f, mSize.x() - 1 - border,
                   mSize.y() - 1 - border, 3);

    if (mEditable && focused())
        mValidFormat ? nvgFillPaint(ctx, fg1) : nvgFillPaint(ctx, fg2);
    else if (mSpinnable && mMouseDownPos.x() != -1)
        nvgFillPaint(ctx, fg1);
    else
        nvgFillPaint(ctx, bg);

    nvgFill(ctx);

    nvgBeginPath(ctx);
    if (border) {
      nvgStrokeWidth(ctx, border);
      nvgRoundedRect(ctx, mPos.x() + 0.5f, mPos.y() + 0.5f, mSize.x() - border,
                   mSize.y() - border, 2.5f);
      nvgStrokeColor(ctx, Color(0, 48));
      nvgStroke(ctx);
      nvgStrokeWidth(ctx, 1.0);
  }

    nvgFontSize(ctx, fontSize());
    nvgFontFace(ctx, "sans");
    Vector2i drawPos(mPos.x(), mPos.y() + mSize.y() * 0.5f + 1);

    float xSpacing = 4.0; //mSize.y() * 0.3f;

    float unitWidth = 0;

    if (mUnitsImage > 0) {
        int w, h;
        nvgImageSize(ctx, mUnitsImage, &w, &h);
        float unitHeight = mSize.y() * 0.4f;
        unitWidth = w * unitHeight / h;
        NVGpaint imgPaint = nvgImagePattern(
            ctx, mPos.x() + mSize.x() - xSpacing - unitWidth,
            drawPos.y() - unitHeight * 0.5f, unitWidth, unitHeight, 0,
            mUnitsImage, mEnabled ? 0.7f : 0.35f);
        nvgBeginPath(ctx);
        nvgRect(ctx, mPos.x() + mSize.x() - xSpacing - unitWidth,
                drawPos.y() - unitHeight * 0.5f, unitWidth, unitHeight);
        nvgFillPaint(ctx, imgPaint);
        nvgFill(ctx);
        unitWidth += 2;
    } else if (!mUnits.empty()) {
        unitWidth = nvgTextBounds(ctx, 0, 0, mUnits.c_str(), nullptr, nullptr);
        nvgFillColor(ctx, Color(255, mEnabled ? 64 : 32));
        nvgTextAlign(ctx, NVG_ALIGN_RIGHT | NVG_ALIGN_MIDDLE);
        nvgText(ctx, mPos.x() + mSize.x() - xSpacing, drawPos.y(),
                mUnits.c_str(), nullptr);
        unitWidth += 2;
    }

    float spinArrowsWidth = 0.f;

    if (mSpinnable && !focused()) {
        spinArrowsWidth = 14.f;

        nvgFontFace(ctx, "icons");
        nvgFontSize(ctx, ((mFontSize < 0) ? mTheme->mButtonFontSize : mFontSize) * 1.2f);

        bool spinning = mMouseDownPos.x() != -1;

        /* up button */ {
            bool hover = mMouseFocus && spinArea(mMousePos) == SpinArea::Top;
            nvgFillColor(ctx, (mEnabled && (hover || spinning)) ? mTheme->mTextColor : mTheme->mDisabledTextColor);
            auto icon = utf8(ENTYPO_ICON_CHEVRON_UP);
            nvgTextAlign(ctx, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
            Vector2f iconPos(mPos.x() + 4.f,
                             mPos.y() + mSize.y()/2.f - xSpacing/2.f);
            nvgText(ctx, iconPos.x(), iconPos.y(), icon.data(), nullptr);
        }

        /* down button */ {
            bool hover = mMouseFocus && spinArea(mMousePos) == SpinArea::Bottom;
            nvgFillColor(ctx, (mEnabled && (hover || spinning)) ? mTheme->mTextColor : mTheme->mDisabledTextColor);
            auto icon = utf8(ENTYPO_ICON_CHEVRON_DOWN);
            nvgTextAlign(ctx, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
            Vector2f iconPos(mPos.x() + 4.f,
                             mPos.y() + mSize.y()/2.f + xSpacing/2.f + 1.5f);
            nvgText(ctx, iconPos.x(), iconPos.y(), icon.data(), nullptr);
        }

        nvgFontSize(ctx, fontSize());
        nvgFontFace(ctx, "sans");
    }

    switch (mAlignment) {
        case Alignment::Left:
            nvgTextAlign(ctx, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
            drawPos.x() += xSpacing + spinArrowsWidth;
            break;
        case Alignment::Right:
            nvgTextAlign(ctx, NVG_ALIGN_RIGHT | NVG_ALIGN_MIDDLE);
            drawPos.x() += mSize.x() - unitWidth - xSpacing;
            break;
        case Alignment::Center:
            nvgTextAlign(ctx, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
            drawPos.x() += mSize.x() * 0.5f;
            break;
    }

    nvgFontSize(ctx, fontSize());
    nvgFillColor(ctx,
                 mEnabled ? mTheme->mTextColor : mTheme->mDisabledTextColor);

    // clip visible text area
    float clipX = mPos.x() + xSpacing + spinArrowsWidth - 1.0f;
    float clipY = mPos.y() + 1.0f;
    float clipWidth = mSize.x() - unitWidth - spinArrowsWidth - 2 * xSpacing + 2.0f;
    float clipHeight = mSize.y() - 3.0f;

    nvgSave(ctx);
    nvgIntersectScissor(ctx, clipX, clipY, clipWidth, clipHeight);
    // mValueTemp is used for display while editing is open
    std::string valStr(mValueTemp);
    if (mCommitted) valStr = mValue;
    if (mCommitted) {
      float scale = value_scale;
      if (scale == 0.0f) scale = 1.0f;
      const char *p = valStr.c_str();
      while (*p && (!isdigit(*p) || *p=='0')) ++p;
      if (format_string.length()) {
        if (value_type == Value::t_integer) {// integer
          char buf[20];
          long val = std::atol(p);
          snprintf(buf, 20, format_string.c_str(), (long)(val / scale));
          valStr = buf;
        }
        else if (value_type == Value::t_float) {
          char buf[20];
          float val = std::atof(p);
          snprintf(buf, 20, format_string.c_str(), val / scale);
          valStr = buf;       
        }
      }
      else if (value_type == Value::t_float) {
        char buf[20];
        float val = std::atof(p);
        snprintf(buf, 20, "%5.3f", val / scale);
        valStr = buf;       
      }
      else if (value_type == Value::t_integer) {
        char buf[20];
        long val = std::atol(p);
        snprintf(buf, 20, "%ld", (long)(val / scale));
        valStr = buf;
      }
    }

    Vector2i oldDrawPos(drawPos);
    drawPos.x() += mTextOffset;

    if (mCommitted) {
        nvgText(ctx, drawPos.x(), drawPos.y(), valStr.c_str(), nullptr);
    } else {
        const int maxGlyphs = 1024;
        NVGglyphPosition glyphs[maxGlyphs];
        float textBound[4];
        nvgTextBounds(ctx, drawPos.x(), drawPos.y(), valStr.c_str(),
                      nullptr, textBound);
        float lineh = textBound[3] - textBound[1];

        // find cursor positions
        int nglyphs =
            nvgTextGlyphPositions(ctx, drawPos.x(), drawPos.y(),
                                  valStr.c_str(), nullptr, glyphs, maxGlyphs);
        updateCursor(ctx, textBound[2], glyphs, nglyphs);

        // compute text offset
        int prevCPos = mCursorPos > 0 ? mCursorPos - 1 : 0;
        int nextCPos = mCursorPos < nglyphs ? mCursorPos + 1 : nglyphs;
        float prevCX = cursorIndex2Position(prevCPos, textBound[2], glyphs, nglyphs);
        float nextCX = cursorIndex2Position(nextCPos, textBound[2], glyphs, nglyphs);

        if (nextCX > clipX + clipWidth)
            mTextOffset -= nextCX - (clipX + clipWidth) + 1;
        if (prevCX < clipX)
            mTextOffset += clipX - prevCX + 1;

        drawPos.x() = oldDrawPos.x() + mTextOffset;

        // draw text with offset
        nvgText(ctx, drawPos.x(), drawPos.y(), valStr.c_str(), nullptr);
        nvgTextBounds(ctx, drawPos.x(), drawPos.y(), valStr.c_str(),
                      nullptr, textBound);

        // recompute cursor positions
        nglyphs = nvgTextGlyphPositions(ctx, drawPos.x(), drawPos.y(),
                valStr.c_str(), nullptr, glyphs, maxGlyphs);

        if (mCursorPos > -1) {
            if (mSelectionPos > -1) {
                float caretx = cursorIndex2Position(mCursorPos, textBound[2],
                                                    glyphs, nglyphs);
                float selx = cursorIndex2Position(mSelectionPos, textBound[2],
                                                  glyphs, nglyphs);

                if (caretx > selx)
                    std::swap(caretx, selx);

                // draw selection
                nvgBeginPath(ctx);
                nvgFillColor(ctx, nvgRGBA(160, 255, 160, 255));
                nvgRect(ctx, caretx, drawPos.y() - lineh * 0.5f, selx - caretx,
                        lineh);
                nvgFill(ctx);
            }

            float caretx = cursorIndex2Position(mCursorPos, textBound[2], glyphs, nglyphs);

            // draw cursor
            nvgBeginPath(ctx);
            nvgMoveTo(ctx, caretx, drawPos.y() - lineh * 0.5f);
            nvgLineTo(ctx, caretx, drawPos.y() + lineh * 0.5f);
            nvgStrokeColor(ctx, nvgRGBA(255, 192, 0, 255));
            nvgStrokeWidth(ctx, 1.0f);
            nvgStroke(ctx);
        }

     nvgFillColor(ctx,
                 mEnabled ? mTheme->mTextColor : mTheme->mDisabledTextColor);
        // draw text with offset
        nvgText(ctx, drawPos.x(), drawPos.y(), valStr.c_str(), nullptr);
        nvgTextBounds(ctx, drawPos.x(), drawPos.y(), valStr.c_str(),
                      nullptr, textBound);

    }
    nvgRestore(ctx);
  if (mSelected)
    drawSelectionBorder(ctx, mPos, mSize);
  else if (EDITOR->isEditMode()) {
    drawElementBorder(ctx, mPos, mSize);
  }

}

