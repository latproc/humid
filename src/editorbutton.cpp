//
//  EditorButton.cpp
//  Project: humid
//
//	All rights reserved. Use of this source code is governed by the
//	3-clause BSD License in LICENSE.txt.

#include <iostream>
#include <nanogui/theme.h>
#include <nanogui/opengl.h>
#include <nanogui/widget.h>

#include "editorwidget.h"
#include "editorbutton.h"
#include "linkableproperty.h"
#include "editor.h"
#include "editorgui.h"
#include "structure.h"

std::string stripEscapes(const std::string &s);

void EditorButton::setupButtonCallbacks(LinkableProperty *lp, EditorGUI *egui) {
  if (!getDefinition()) return;
  std::string conn("");
  if (getDefinition()->getKind() == "BUTTON") {
    EditorGUI *gui = egui;
    if (lp) {
      lp->link(new LinkableIndicator(this));
      setRemote(lp);
    }
    if (getRemote()) 
      conn = getRemote()->group();
    else
      conn = getConnection();
      //if (!flags() & nanogui::Button::SetOnButton || flags() & nanogui::Button::SetOffButton)
      //  getRemote()->link(new LinkableIndicator(this));
    
    std::string cmd = command();
    setCallback([&,this, gui, conn, cmd] {
      if (cmd.length()) {
          std::cout << name << " sending " << cmd << " to " << conn << "\n";
          gui->queueMessage(conn, cmd,
            [](std::string s){std::cout << " Response: " << s << "\n"; });
      }
      else if (flags() & nanogui::Button::RemoteButton) {
        std::string msgon = gui->getIODSyncCommand(conn, 0, address(), pushed() ? 1 : 0);
            gui->queueMessage(conn, msgon, [](std::string s){std::cout << ": " << s << "\n"; });
      }
      if (flags() & nanogui::Button::NormalButton && !(flags() & nanogui::Button::RemoteButton)) {
        if (getRemote()) {
            std::string msgon = gui->getIODSyncCommand(conn, 0, address(), 1);
            gui->queueMessage(conn, msgon, [](std::string s){std::cout << ": " << s << "\n"; });
            std::string msgoff = gui->getIODSyncCommand(conn, 0, address(), 0);
            gui->queueMessage(conn, msgoff, [](std::string s){std::cout << ": " << s << "\n"; });
          }
      }

    });
    setChangeCallback([&,this,gui, conn] (bool state) {
      const std::string &conn = getRemote()->group();
      if (getRemote()) {
        if ( !(flags() & nanogui::Button::NormalButton ) )  {
          if (flags() & nanogui::Button::SetOnButton || flags() & nanogui::Button::SetOffButton) { 
            gui->queueMessage(conn,
              gui->getIODSyncCommand(conn, getRemote()->getKind(), address(), state), [](std::string s){std::cout << s << "\n"; });
          }
          else
            gui->queueMessage(conn,
              gui->getIODSyncCommand(conn, getRemote()->getKind(), address(),(state)?1:0), [](std::string s){std::cout << s << "\n"; });
        }
      }
    });
  }
  else {
    if (lp)
      lp->link(new LinkableIndicator(this));
    setChangeCallback(nullptr); //[b, this] (bool state) { });
    setCallback(nullptr);
  }
}


EditorButton::EditorButton(NamedObject *owner, Widget *parent, const std::string &btn_name, LinkableProperty *lp, const std::string &caption,
            bool toggle, int icon)
	: Button(parent, caption, icon), EditorWidget(owner, "BUTTON", btn_name, this, lp), is_toggle(toggle), 
    alignment(1), valign(1), wrap_text(false), shadow(1) {
    setPushed(false);
    bg_on_color = nanogui::Color(0.3f, 0.3f, 0.3f, 0.0f);
    on_text_colour = mTextColor;
    alignment = NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE;
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

void EditorButton::getPropertyNames(std::list<std::string> &names) {
    EditorWidget::getPropertyNames(names);
		names.push_back("Off text");
    names.push_back("On text");
    names.push_back("Background colour");
    names.push_back("Background on colour");
    names.push_back("Text colour");
    names.push_back("Text on colour");
    names.push_back("Behaviour");
    names.push_back("Command");
    names.push_back("Vertical Alignment");
    names.push_back("Alignment");
    names.push_back("Wrap Text");
}

void EditorButton::loadPropertyToStructureMap(std::map<std::string, std::string> &property_map) {
  EditorWidget::loadPropertyToStructureMap(property_map);
  property_map["Off text"] = "caption";
  property_map["On text"] = "on_caption";
  property_map["Background colour"] = "bg_color";
  property_map["Background on colour"] = "bg_on_color";
  property_map["Text colour"] = "text_colour";
  property_map["Text on colour"] = "on_text_colour";
  property_map["Behaviour"] = "behaviour";
  property_map["Command"] = "command";
  property_map["Alignment"] = "alignment";
  property_map["Vertical Alignment"] = "valign";
  property_map["Wrap Text"] = "wrap";
}

Value EditorButton::getPropertyValue(const std::string &prop) {
  Value res = EditorWidget::getPropertyValue(prop);
  if (res != SymbolTable::Null)
    return res;

  if (prop == "Off text")
    return Value(caption(), Value::t_string);
  if (prop == "On text")
    return Value(on_caption, Value::t_string);
  if (prop == "Background colour") {
    nanogui::Widget *w = dynamic_cast<nanogui::Widget*>(this);
    nanogui::Button *btn = dynamic_cast<nanogui::Button*>(this);
    char buf[50];
    snprintf(buf, 50, "%5.4f,%5.4f,%5.4f,%5.4f",
      backgroundColor().r(), backgroundColor().g(), backgroundColor().b(), backgroundColor().w());
    return Value(buf, Value::t_string);
  }
  if (prop == "Background on colour") {
    nanogui::Widget *w = dynamic_cast<nanogui::Widget*>(this);
    nanogui::Button *btn = dynamic_cast<nanogui::Button*>(this);
    char buf[50];
    snprintf(buf, 50, "%5.4f,%5.4f,%5.4f,%5.4f",
      bg_on_color.r(), bg_on_color.g(), bg_on_color.b(), bg_on_color.w());
    return Value(buf, Value::t_string);
  }
  if (prop == "Text colour") {
    nanogui::Widget *w = dynamic_cast<nanogui::Widget*>(this);
    char buf[50];
    snprintf(buf, 50, "%5.4f,%5.4f,%5.4f,%5.4f", mTextColor.r(), mTextColor.g(), mTextColor.b(), mTextColor.w());
    return Value(buf, Value::t_string);
  }
  if (prop == "Text on colour") {
    nanogui::Widget *w = dynamic_cast<nanogui::Widget*>(this);
    char buf[50];
    snprintf(buf, 50, "%5.4f,%5.4f,%5.4f,%5.4f", on_text_colour.r(), on_text_colour.g(), on_text_colour.b(), on_text_colour.w());
    return Value(buf, Value::t_string);
  }
  if (prop == "Command") {
    return Value(command(), Value::t_string);
  }
  if (prop == "Behaviour") {
    return flags();
  }
  if (prop == "Alignment") return alignment;
  if (prop == "Vertical Alignment") return valign;
  if (prop == "Wrap Text") return wrap_text ? 1 : 0;
  return SymbolTable::Null;
}

void EditorButton::setProperty(const std::string &prop, const std::string value) {
  EditorWidget::setProperty(prop, value);
  if (prop == "Remote") {
    if (remote) {
      if (getDefinition()->getKind() == "INDICATOR")
        remote->link(new LinkableIndicator(this));  }
    }
    if (prop == "Alignment") alignment = std::atoi(value.c_str());
    if (prop == "Vertical Alignment") valign = std::atoi(value.c_str());
    if (prop == "Wrap Text") {
      wrap_text = (value == "1" || value == "true" || value == "TRUE");
    }
}


void EditorButton::draw(NVGcontext *ctx) {
    using namespace nanogui;

    Widget::draw(ctx);
    NVGcolor gradTop = mTheme->mButtonGradientTopFocused;
    NVGcolor gradBot = mTheme->mButtonGradientBotFocused;

    if (mPushed) {
        gradTop = mTheme->mButtonGradientTopPushed;
        gradBot = mTheme->mButtonGradientBotPushed;
    } else {
        gradTop = mTheme->mButtonGradientTopFocused;
        gradBot = mTheme->mButtonGradientBotFocused;
    }

    nvgBeginPath(ctx);

    nvgRoundedRect(ctx, mPos.x() + 1, mPos.y() + 1.0f, mSize.x() - 2,
                 mSize.y() - 2, mTheme->mButtonCornerRadius - 1);

    if (mPushed) {
      if (bg_on_color.w() != 0) {
        nvgFillColor(ctx, Color(bg_on_color.head<3>(), 1.f));
        nvgFill(ctx);
        gradTop.a = gradBot.a = 0.0f;
      }  
      else {
        nvgFillColor(ctx, Color(mBackgroundColor.head<3>(), 0.4f));
        nvgFill(ctx);
        double v = 1 - mBackgroundColor.w();
        gradTop.a = gradBot.a = mEnabled ? v : v; // * .5f + .5f;
      }  
    }
    else if (mBackgroundColor.w() != 0) {
      nvgFillColor(ctx, Color(mBackgroundColor.head<3>(), 1.f));
      nvgFill(ctx);
      double v = 1 - mBackgroundColor.w();
      gradTop.a = gradBot.a = mEnabled ? v : v; // * .5f + .5f;
    }

    NVGpaint bg = nvgLinearGradient(ctx, mPos.x(), mPos.y(), mPos.x(),
                                    mPos.y() + mSize.y(), gradTop, gradBot);

    nvgFillPaint(ctx, bg);
    nvgFill(ctx);

    nvgBeginPath(ctx);
    nvgStrokeWidth(ctx, border);
    nvgRoundedRect(ctx, mPos.x() + 0.5f, mPos.y() + (mPushed ? 0.5f : 1.5f), mSize.x() - 1,
                   mSize.y() - 1 - (mPushed ? 0.0f : 1.0f), mTheme->mButtonCornerRadius);
    nvgStrokeColor(ctx, mTheme->mBorderLight);
    nvgStroke(ctx);

    nvgBeginPath(ctx);
    nvgRoundedRect(ctx, mPos.x() + 0.5f, mPos.y() + 0.5f, mSize.x() - 1,
                   mSize.y() - 2, mTheme->mButtonCornerRadius);
    nvgStrokeColor(ctx, mTheme->mBorderDark);
    nvgStroke(ctx);
 
    int fontSize = mFontSize == -1 ? mTheme->mButtonFontSize : mFontSize;
    nvgFontSize(ctx, fontSize);
    nvgFontFace(ctx, "sans-bold");

    NVGcolor textColor =
        mTextColor.w() == 0 ? mTheme->mTextColor : mTextColor;

    std::string text = mCaption;
    if (mPushed && on_text_colour.w() != 0) {
      if (!on_caption.empty()) text = on_caption;
      textColor = on_text_colour;
    }
    float tw = nvgTextBounds(ctx, 0,0, text.c_str(), nullptr, nullptr);

    Vector2f center = mPos.cast<float>() + mSize.cast<float>() * 0.5f;
    Vector2f textPos(center.x() - tw * 0.5f, center.y() - 1);

    //if (!mEnabled)
    //    textColor = mTheme->mDisabledTextColor;

    if (mIcon) {
        auto icon = utf8(mIcon);

        float iw, ih = fontSize;
        if (nvgIsFontIcon(mIcon)) {
            ih *= 1.5f;
            if (mIconPosition == IconPosition::Filled) { ih=mSize.y(); }
            nvgFontSize(ctx, ih);
            nvgFontFace(ctx, "icons");
            iw = nvgTextBounds(ctx, 0, 0, icon.data(), nullptr, nullptr);
        } else {
            int w, h;
            ih *= 0.9f;
            if (mIconPosition == IconPosition::Filled) { ih = mSize.y(); }
            nvgImageSize(ctx, mIcon, &w, &h);
            iw = w * ih / h;
        }
        if (mIconPosition != IconPosition::Filled && text != "")
            iw += mSize.y() * 0.15f;
        nvgFillColor(ctx, textColor);
        nvgTextAlign(ctx, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
        Vector2f iconPos = center;
        iconPos.y() -= 1;

        if (mIconPosition == IconPosition::LeftCentered) {
            iconPos.x() -= (tw + iw) * 0.5f;
            textPos.x() += iw * 0.5f;
        } else if (mIconPosition == IconPosition::RightCentered) {
            textPos.x() -= iw * 0.5f;
            iconPos.x() += tw * 0.5f;
        } else if (mIconPosition == IconPosition::Left) {
            iconPos.x() = mPos.x() + 8;
        } else if (mIconPosition == IconPosition::Right) {
            iconPos.x() = mPos.x() + mSize.x() - iw - 8;
        }
        else if (mIconPosition == IconPosition::Filled) {
            iconPos.x() = mPos.x(); iconPos.y() = mPos.y() + ih/2; // compensates for offset below
        }

        if (nvgIsFontIcon(mIcon)) {
            nvgText(ctx, iconPos.x(), iconPos.y()+1, icon.data(), nullptr);
        } else {
            NVGpaint imgPaint = nvgImagePattern(ctx,
                    iconPos.x(), iconPos.y() - ih/2, iw, ih, 0, mIcon, mEnabled ? 0.5f : 0.25f);

            nvgFillPaint(ctx, imgPaint);
            nvgFill(ctx);
        }
    }

    // we are using two separate alignments: 1,2,4 so we 
    // convert the default alignment from nanogui here
    if (alignment == (NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE) ) {
      alignment = 1;
      valign = NVG_ALIGN_CENTER;
    }
    nvgFontSize(ctx, fontSize);
    nvgFontFace(ctx, "sans-bold");
    nvgTextAlign(ctx, alignment);
    int label_x = textPos.x();
    int label_y = textPos.y();
    int align = NVG_ALIGN_LEFT;

    if (alignment & 2) {
      label_x = mPos.x() + mSize.x()-5;
      align = NVG_ALIGN_RIGHT;
    }
    else if (alignment & 1) {
      label_x = mPos.x() + mSize.x()/2;
      align = NVG_ALIGN_CENTER;
    }
    else
      label_x = mPos.x();
    
    if (wrap_text) {
      if (alignment & NVG_ALIGN_RIGHT) {
        label_x = mPos.x() + mSize.x()-5;
        align = NVG_ALIGN_RIGHT;
      }
      else if (alignment & NVG_ALIGN_CENTER) {
        label_x = textPos.x();
        align = NVG_ALIGN_CENTER;
      }
      else
        label_x = mPos.x();
    }
    int alignv = NVG_ALIGN_MIDDLE;
    if (valign & NVG_ALIGN_LEFT)
      alignv = NVG_ALIGN_TOP;
    else if (valign & NVG_ALIGN_RIGHT)
      alignv = NVG_ALIGN_BOTTOM;


    nvgTextAlign(ctx, align | alignv);
    if (shadow) {
      nvgFillColor(ctx, mTheme->mTextColorShadow);
      if (!wrap_text)
        nvgText(ctx, label_x, label_y, text.c_str(), nullptr);
      else {
        nvgTextBox(ctx, mPos.x(), mPos.y()+mSize.y()/2, mSize.x(), text.c_str(), nullptr);
      }
    }
    nvgFillColor(ctx, textColor);
    if (!wrap_text)
      nvgText(ctx, label_x, label_y + 1, text.c_str(), nullptr);
    else {
      nvgTextBox(ctx, mPos.x(), mPos.y()+mSize.y()/2, mSize.x(), text.c_str(), nullptr);
    }
    if (mSelected)
      drawSelectionBorder(ctx, mPos, mSize);
    else if (EDITOR->isEditMode()) {
      drawElementBorder(ctx, mPos, mSize);
    }


}
