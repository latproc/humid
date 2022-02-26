//
//  EditorButton.cpp
//  Project: humid
//
//	All rights reserved. Use of this source code is governed by the
//	3-clause BSD License in LICENSE.txt.

#include <iostream>
#include <nanogui/opengl.h>
#include <nanogui/theme.h>
#include <nanogui/widget.h>
#include <set>
#include <utility>

#include "colourhelper.h"
#include "editor.h"
#include "editorbutton.h"
#include "editorgui.h"
#include "editorwidget.h"
#include "helper.h"
#include "linkableproperty.h"
#include "propertyformhelper.h"
#include "resourcemanager.h"
#include "structure.h"
#include "valuehelper.h"

std::string stripEscapes(const std::string &s);

const std::map<std::string, std::string> &EditorButton::property_map() const {
    auto structure_class = findClass("BUTTON");
    assert(structure_class);
    return structure_class->property_map();
}

const std::map<std::string, std::string> &EditorButton::reverse_property_map() const {
    auto structure_class = findClass("BUTTON");
    assert(structure_class);
    return structure_class->reverse_property_map();
}

namespace {

int intFromHorizontalAlignment(EditorButton::HorizontalAlignment align) {
    switch (align) {
    case EditorButton::HorizontalAlignment::Left:
        return NVG_ALIGN_LEFT;
    case EditorButton::HorizontalAlignment::Centre:
        return NVG_ALIGN_CENTER;
    case EditorButton::HorizontalAlignment::Right:
        return NVG_ALIGN_RIGHT;
    }
    return NVG_ALIGN_CENTER;
}

std::string fromHorizontalAlignment(EditorButton::HorizontalAlignment align) {
    switch (align) {
    case EditorButton::HorizontalAlignment::Left:
        return "left";
    case EditorButton::HorizontalAlignment::Centre:
        return "centre";
    case EditorButton::HorizontalAlignment::Right:
        return "right";
    }
    return "centre";
}

EditorButton::HorizontalAlignment toHorizontalAlignment(int align) {
    if (align == static_cast<int>(EditorButton::HorizontalAlignment::Centre)) {
        return EditorButton::HorizontalAlignment::Centre;
    }
    else if (align == static_cast<int>(EditorButton::HorizontalAlignment::Left)) {
        return EditorButton::HorizontalAlignment::Left;
    }
    else if (align == static_cast<int>(EditorButton::HorizontalAlignment::Right)) {
        return EditorButton::HorizontalAlignment::Right;
    }
    return EditorButton::HorizontalAlignment::Centre;
}

EditorButton::HorizontalAlignment toHorizontalAlignment(const std::string &align) {
    if (align == "left") {
        return EditorButton::HorizontalAlignment::Left;
    }
    if (align == "centre" || align == "center") {
        return EditorButton::HorizontalAlignment::Centre;
    }
    if (align == "right") {
        return EditorButton::HorizontalAlignment::Right;
    }
    return toHorizontalAlignment(std::atoi(align.c_str()));
}

int intFromVerticalAlignment(EditorButton::VerticalAlignment align) {
    switch (align) {
    case EditorButton::VerticalAlignment::Top:
        return NVG_ALIGN_LEFT;
    case EditorButton::VerticalAlignment::Centre:
        return NVG_ALIGN_CENTER;
    case EditorButton::VerticalAlignment::Bottom:
        return NVG_ALIGN_RIGHT;
    }
    return NVG_ALIGN_CENTER;
}

std::string fromVerticalAlignment(EditorButton::VerticalAlignment align) {
    switch (align) {
    case EditorButton::VerticalAlignment::Top:
        return "top";
    case EditorButton::VerticalAlignment::Centre:
        return "centre";
    case EditorButton::VerticalAlignment::Bottom:
        return "bottom";
    }
    return "centre";
}

EditorButton::VerticalAlignment toVerticalAlignment(int align) {
    if (align == static_cast<int>(EditorButton::VerticalAlignment::Top)) {
        return EditorButton::VerticalAlignment::Top;
    }
    else if (align == static_cast<int>(EditorButton::VerticalAlignment::Centre)) {
        return EditorButton::VerticalAlignment::Centre;
    }
    else if (align == static_cast<int>(EditorButton::VerticalAlignment::Bottom)) {
        return EditorButton::VerticalAlignment::Bottom;
    }
    return EditorButton::VerticalAlignment::Centre;
}

EditorButton::VerticalAlignment toVerticalAlignment(const std::string &align) {
    if (align == "top") {
        return EditorButton::VerticalAlignment::Top;
    }
    if (align == "middle" || align == "centre" || align == "center") {
        return EditorButton::VerticalAlignment::Centre;
    }
    if (align == "bottom") {
        return EditorButton::VerticalAlignment::Bottom;
    }
    return toVerticalAlignment(std::atoi(align.c_str()));
}
} // namespace
void EditorButton::setupButtonCallbacks(LinkableProperty *lp, EditorGUI *egui) {
    if (!getDefinition())
        return;
    std::string conn("");
    if (!getDefinition()->isA("INDICATOR")) {
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
        setCallback([&, this, gui, conn, cmd] {
            if (cmd.length()) {
                extern int debug;
                if (debug) {
                    std::cout << name << " sending: " << cmd << " to " << conn << "\n";
                }
                gui->queueMessage(conn, cmd, [](std::string s) {
                    extern int debug;
                    if (debug) {
                        std::cout << " Response: " << s << "\n";
                    }
                });
            }
            else if (flags() & nanogui::Button::RemoteButton) {
                // a remote button is reset remotely
                std::string msgon = gui->getIODSyncCommand(conn, 0, address(), pushed() ? 1 : 0);
                gui->queueMessage(conn, msgon,
                                  [](std::string s) { std::cout << ": " << s << "\n"; });
            }
            if (flags() & nanogui::Button::NormalButton &&
                !(flags() & nanogui::Button::RemoteButton)) {
                if (getRemote()) {
                    // a normal button is pressed and released
                    std::string msgon = gui->getIODSyncCommand(conn, 0, address(), 1);
                    gui->queueMessage(conn, msgon,
                                      [](std::string s) {
                                          extern int debug;
                                          if (debug) { std::cout << ": " << s << "\n"; }
                                        });
                    std::string msgoff = gui->getIODSyncCommand(conn, 0, address(), 0);
                    gui->queueMessage(conn, msgoff,
                                      [](std::string s) {
                                          extern int debug;
                                          if (debug) { std::cout << ": " << s << "\n"; }
                                        });
                }
            }
        });
        setChangeCallback([&, this, gui, conn](bool state) {
            const std::string &conn = getRemote()->group();
            if (getRemote()) {
                if (!(flags() & nanogui::Button::NormalButton)) {
                    if (flags() & nanogui::Button::SetOnButton ||
                        flags() & nanogui::Button::SetOffButton) {
                        gui->queueMessage(
                            conn,
                            gui->getIODSyncCommand(conn, getRemote()->getKind(), address(), state),
                            [](std::string s) {
                                          extern int debug;
                                          if (debug) { std::cout << ": " << s << "\n"; }
                                        });
                    }
                    else
                        gui->queueMessage(conn,
                                        gui->getIODSyncCommand(conn, getRemote()->getKind(),
                                                                 address(), (state) ? 1 : 0),
                                        [](std::string s) {
                                            extern int debug;
                                            if (debug) { std::cout << ": " << s << "\n"; }
                                        });
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

EditorButton::EditorButton(NamedObject *owner, Widget *parent, const std::string &btn_name,
                           LinkableProperty *lp, const std::string &caption, bool toggle, int icon)
    : Button(parent, caption, icon), EditorWidget(owner, "BUTTON", btn_name, this, lp),
      is_toggle(toggle), alignment(HorizontalAlignment::Centre), valign(VerticalAlignment::Centre),
      wrap_text(false), shadow(1) {
    setPushed(false);
    bg_on_color = mBackgroundColor;
    text_on_colour = mTextColor;
}

EditorButton::~EditorButton() {
    if (mImageID)
        ResourceManager::release(mImageID);
}

void EditorButton::setImageName(const std::string &image) {
    if (mImageID) {
        ResourceManager::release(mImageID);
        mImageID = 0;
    }
    image_name =
        image.empty() || image == "null" ? SymbolTable::Null : Value(image, Value::t_string);
    getDefinition()->getProperties().add("image", image_name);
}

std::string EditorButton::imageName() const {
    return image_name == SymbolTable::Null ? "" : image_name.asString();
}

bool EditorButton::mouseButtonEvent(const nanogui::Vector2i &p, int button, bool down,
                                    int modifiers) {

    using namespace nanogui;
    if (editorMouseButtonEvent(this, p, button, down, modifiers)) {
        //if (!is_toggle || (is_toggle && down))
        return nanogui::Button::mouseButtonEvent(p, button, down, modifiers);
    }

    return true;
}

bool EditorButton::mouseMotionEvent(const nanogui::Vector2i &p, const nanogui::Vector2i &rel,
                                    int button, int modifiers) {

    if (editorMouseMotionEvent(this, p, rel, button, modifiers))
        return Button::mouseMotionEvent(p, rel, button, modifiers);

    return true;
}

bool EditorButton::mouseEnterEvent(const Vector2i &p, bool enter) {
    if (editorMouseEnterEvent(this, p, enter))
        return Button::mouseEnterEvent(p, enter);

    return true;
}

void EditorButton::setCommand(std::string cmd) { command_str = stripEscapes(cmd); }

const std::string &EditorButton::command() const { return command_str; }

void EditorButton::getPropertyNames(std::list<std::string> &names) {
    EditorWidget::getPropertyNames(names);
    names.push_back("Off text");
    names.push_back("On text");
    names.push_back("Background Colour");
    names.push_back("Background on colour");
    names.push_back("Border colouring");
    names.push_back("Border gradient dir");
    names.push_back("Border grad bot");
    names.push_back("Border grad top");
    names.push_back("Border style");
    names.push_back("Text colour");
    names.push_back("Text on colour");
    names.push_back("Behaviour");
    names.push_back("Command");
    names.push_back("Vertical Alignment");
    names.push_back("Alignment");
    names.push_back("Wrap Text");
    names.push_back("Image");
    names.push_back("Image opacity");
}

void EditorButton::loadPropertyToStructureMap(std::map<std::string, std::string> &properties) {
    properties = property_map();
}

Value EditorButton::getPropertyValue(const std::string &prop) {
    Value res = EditorWidget::getPropertyValue(prop);
    if (res != SymbolTable::Null)
        return res;

    else if (prop == "Off text") {
        return Value(caption(), Value::t_string);
    }
    else if (prop == "Image") {
        return image_name;
    }
    else if (prop == "On text") {
        return Value(on_caption, Value::t_string);
    }
    else if (prop == "Background Colour") {
        nanogui::Widget *w = dynamic_cast<nanogui::Widget *>(this);
        nanogui::Button *btn = dynamic_cast<nanogui::Button *>(this);
        return Value(stringFromColour(backgroundColor()), Value::t_string);
    }
    else if (prop == "Command") {
        return Value(command(), Value::t_string);
    }
    else if (prop == "Behaviour") {
        return flags();
    }
    else if (prop == "Alignment")
        return fromHorizontalAlignment(alignment);
    else if (prop == "Vertical Alignment")
        return fromVerticalAlignment(valign);
    else if (prop == "Wrap Text")
        return wrap_text ? 1 : 0;
    else if (prop == "Border colouring") {
        return static_cast<int>(border_colouring);
    }
    else if (prop == "Border style") {
        return static_cast<int>(border_style);
    }
    else if (prop == "Border gradient dir") {
        return static_cast<int>(border_grad_dir);
    }
    else if (prop == "Border grad bot") {
        return Value(stringFromColour(border_grad_bot), Value::t_string);
    }
    else if (prop == "Border grad top") {
        return Value(stringFromColour(border_grad_top), Value::t_string);
    }
    else if (prop == "Background on colour") {
        return Value(stringFromColour(bg_on_color), Value::t_string);
    }
    else if (prop == "Text colour") {
        return Value(stringFromColour(mTextColor), Value::t_string);
    }
    else if (prop == "Text on colour") {
        return Value(stringFromColour(text_on_colour), Value::t_string);
    }
    return SymbolTable::Null;
}

void EditorButton::setProperty(const std::string &prop, const std::string value) {
    EditorWidget::setProperty(prop, value);
    if (prop == "Remote") {
        if (remote) {
            if (getDefinition()->getKind() == "INDICATOR")
                remote->link(new LinkableIndicator(this));
        }
    }
    else if (prop == "On text") {
        on_caption = value;
    }
    else if (prop == "Off text") {
        setCaption(value);
    }
    else if (prop == "Background Colour") {
        getDefinition()->getProperties().add("bg_color", value);
        setBackgroundColor(colourFromProperty(getDefinition(), "bg_color"));
    }
    else if (prop == "Background on colour") {
        getDefinition()->getProperties().add("bg_on_color", value);
        setOnColor(colourFromProperty(getDefinition(), "bg_on_color"));
    }
    else if (prop == "Text colour") {
        getDefinition()->getProperties().add("text_colour", value);
        setTextColor(colourFromProperty(getDefinition(), "text_colour"));
    }
    else if (prop == "Text on colour") {
        getDefinition()->getProperties().add("text_on_colour", value);
        setOnTextColor(colourFromProperty(getDefinition(), "text_on_colour"));
    }
    else if (prop == "Alignment") {
        alignment = toHorizontalAlignment(value);
        // horizontal alignments had incorrect numeric values. The following
        // ensures the symbolic name is saved instead of a bare integer.
        // for a time(?), reading toHorizintalAlignment(int) supports the old value
        getDefinition()->getProperties().add("alignment", fromHorizontalAlignment(alignment));
    }
    else if (prop == "Vertical Alignment")
        valign = toVerticalAlignment(value);
    else if (prop == "Wrap Text") {
        wrap_text = (value == "1" || value == "true" || value == "TRUE");
    }
    else if (prop == "Image") {
        setImageName(value);
    }
    else if (prop == "Image transparency") {
        image_alpha = std::atof(value.c_str());
    }
    if (prop == "Enabled" && getDefinition()->getKind() != "INDICATOR") {
        mEnabled = (value == "1" || value == "true" || value == "TRUE");
        if (!mEnabled) {
            setBackgroundColor(nanogui::Color(0.7f, 0.7f, 0.7f, 1.0f));
            setTextColor(nanogui::Color(0.8f, 0.8f, 0.8f, 1.0f));
        }
        else {
            setBackgroundColor(colourFromProperty(getDefinition(), "bg_color"));
            setTextColor(colourFromProperty(getDefinition(), "text_colour"));
        }
    }
    else if (prop == "Border colouring") {
        Value bc(value);
        long bci = -1;
        if (bc.asInteger(bci)) {
            getDefinition()->getProperties().add("border_colouring", bci);
            border_colouring = static_cast<BorderColouring>(bci);
        }
        else {
            if (value == "manual") {
                getDefinition()->getProperties().add("border_colouring", 0);
                border_colouring = BorderColouring::Manual;
            }
            else if (value == "auto") {
                getDefinition()->getProperties().add("border_colouring", 1);
                border_colouring = BorderColouring::Auto;
            }
        }
    }
    else if (prop == "Border gradient dir") {
        Value bgd(value);
        long bgdi = -1;
        if (bgd.asInteger(bgdi)) {
            getDefinition()->getProperties().add("border_grad_dir", bgdi);
            border_grad_dir = static_cast<BorderGradientDirection>(bgdi);
        }
        else {
            if (value == "down") {
                getDefinition()->getProperties().add("border_grad_dir", 0);
                border_grad_dir = BorderGradientDirection::Down;
            }
            else if (value == "right") {
                getDefinition()->getProperties().add("border_grad_dir", 1);
                border_grad_dir = BorderGradientDirection::Right;
            }
            if (value == "up") {
                getDefinition()->getProperties().add("border_grad_dir", 2);
                border_grad_dir = BorderGradientDirection::Up;
            }
            else if (value == "left") {
                getDefinition()->getProperties().add("border_grad_dir", 3);
                border_grad_dir = BorderGradientDirection::Left;
            }
            else if (value == "tlbr") {
                getDefinition()->getProperties().add("border_grad_dir", 4);
                border_grad_dir = BorderGradientDirection::TLBR;
            }
            else if (value == "brtl") {
                getDefinition()->getProperties().add("border_grad_dir", 4);
                border_grad_dir = BorderGradientDirection::BRTL;
            }
        }
    }
    else if (prop == "Border style") {
        Value bs(value);
        long bsi = -1;
        if (bs.asInteger(bsi)) {
            getDefinition()->getProperties().add("border_style", value);
            border_style = static_cast<BorderStyle>(std::atoi(value.c_str()));
        }
        else {
            if (value == "none") {
                border = 0;
                getDefinition()->getProperties().add("border", 0);
                getDefinition()->getProperties().add("border_style", 0);
                border_style = BorderStyle::None;
            }
            else if (value == "shadow") {
                getDefinition()->getProperties().add("border_style", 1);
                border_style = BorderStyle::Shadow;
            }
            else if (value == "frame") {
                getDefinition()->getProperties().add("border_style", 2);
                border_style = BorderStyle::Frame;
            }
        }
    }
    else if (prop == "Border grad bot") {
        getDefinition()->getProperties().add("border_grad_bot", value);
        border_grad_bot = colourFromProperty(getDefinition(), "bg_on_color");
    }
    else if (prop == "Border grad top") {
        getDefinition()->getProperties().add("border_grad_top", value);
        border_grad_top = colourFromProperty(getDefinition(), "bg_on_color");
    }
}

void EditorButton::draw(NVGcontext *ctx) {
    using namespace nanogui;

    NVGcolor gradTop = mTheme->mButtonGradientTopUnfocused;
    NVGcolor gradBot = mTheme->mButtonGradientBotUnfocused;

    Widget::draw(ctx);

    auto border_gradient_bottom = border_grad_bot;
    auto border_gradient_top = border_grad_top;
    if (border_colouring == BorderColouring::Auto) {
        auto base_colour = mPushed ? bg_on_color : mBackgroundColor;
        float lum =
            0.2126f * base_colour.r() + 0.7152f * base_colour.g() + 0.0722 * base_colour.b();
        float darken = 0.4f;
        float lighten = 0.9f;
        border_gradient_bottom = {darken * base_colour.r(), darken * base_colour.g(),
                                  darken * base_colour.b(), 1.0f};
        border_gradient_top = {lighten * base_colour.r(), lighten * base_colour.g(),
                               lighten * base_colour.b(), 1.0f};
    }

    if (mPushed) {
        gradTop = mTheme->mButtonGradientTopPushed;
        gradBot = mTheme->mButtonGradientBotPushed;
    }
    else if (mMouseFocus && mEnabled) {
        gradTop = mTheme->mButtonGradientTopFocused;
        gradBot = mTheme->mButtonGradientBotFocused;
    }

    if (image_name != SymbolTable::Null && !mImageID) {
        mImageID = nvgCreateImage(ctx, image_name.asString().c_str(), 0);
        if (mImageID) {
            ResourceManager::manage(mImageID);
        }
        else {
            std::cerr << "failed to load image " << image_name << "\n";
        }
    }

    nvgSave(ctx);
    // nanogui::Vector2i offset(0,0);
    // if (mImageID != 0 && mPushed) {
    //   offset = {1,1};
    //   nvgTranslate(ctx, offset.x(), offset.y());
    // }

    nanogui::Vector2i offset(mPos);
    nanogui::Vector2i size(mSize);
    if (border > 0) {
        nvgBeginPath(ctx);
        if (border_style == BorderStyle::Frame) {
            nvgStrokeWidth(ctx, border + 2);
            nvgBeginPath(ctx);
            nvgRoundedRect(ctx, offset.x(), offset.y(), size.x(), size.y(),
                           mTheme->mButtonCornerRadius);
            nanogui::Vector2i border_gradient_end(offset.x(), offset.y() + size.y());
            if (border_grad_dir == BorderGradientDirection::Right ||
                border_grad_dir == BorderGradientDirection::Left) {
                border_gradient_end = {offset.x() + size.x(), offset.y()};
            }
            else if (border_grad_dir == BorderGradientDirection::TLBR ||
                     border_grad_dir == BorderGradientDirection::BRTL) {
                border_gradient_end = {offset.x() + size.x(), offset.y() + size.x()};
            }
            const std::set<BorderGradientDirection> reversed_grad = {BorderGradientDirection::Up,
                                                                     BorderGradientDirection::Left,
                                                                     BorderGradientDirection::BRTL};
            if (reversed_grad.count(border_grad_dir)) {
                std::swap<nanogui::Color>(border_gradient_bottom, border_gradient_top);
            }
            NVGpaint bg = nvgLinearGradient(ctx, offset.x(), offset.y(), border_gradient_end.x(),
                                            border_gradient_end.y(), border_gradient_top,
                                            border_gradient_bottom);
            nvgFillPaint(ctx, bg);
            nvgFill(ctx);
            offset += nanogui::Vector2i(border, border);
            size -= nanogui::Vector2i(2 * border, 2 * border);
        }
        else {
            nvgStrokeWidth(ctx, border + 2);
            nvgRoundedRect(ctx, offset.x(), offset.y(), size.x(), size.y(),
                           mTheme->mButtonCornerRadius);
            nvgFillColor(ctx, Color(mTheme->mBorderDark.head<3>(), 1.f));
            nvgFill(ctx);
            offset += nanogui::Vector2i(border, border);
            size -= nanogui::Vector2i(2 * border, 2 * border);
        }
    }

    if (mPushed) {
        nvgBeginPath(ctx);
        if (border_style == BorderStyle::Frame) {
            offset += nanogui::Vector2i(2, 2);
            size -= nanogui::Vector2i(4, 4);
        }
        nvgRoundedRect(ctx, offset.x(), offset.y(), size.x(), size.y(),
                       mTheme->mButtonCornerRadius);
        if (bg_on_color.w() != 0) {
            nvgFillColor(ctx, Color(bg_on_color.head<3>(), 1.f));
            nvgFill(ctx);
            gradTop.a = gradBot.a = 0.0f;
        }
        else if (mBackgroundColor.w() != 0) {
            nvgFillColor(ctx, Color(mBackgroundColor.head<3>(), 0.4f));
            nvgFill(ctx);
            double v = 1 - mBackgroundColor.w();
            gradTop.a = gradBot.a = mEnabled ? v : v * .5f + .5f;
        }
    }
    else if (mBackgroundColor.w() != 0) {
        nvgBeginPath(ctx);
        nvgRoundedRect(ctx, offset.x(), offset.y(), size.x(), size.y(),
                       mTheme->mButtonCornerRadius);
        nvgFillColor(ctx, Color(mBackgroundColor.head<3>(), 1.f));
        nvgFill(ctx);
        double v = 1 - mBackgroundColor.w();
        gradTop.a = gradBot.a = mEnabled ? v : v * .5f + .5f;
    }

    if (mImageID != 0) {
        nvgBeginPath(ctx);
        nvgRoundedRect(ctx, offset.x(), offset.y(), size.x(), size.y(),
                       mTheme->mButtonCornerRadius);
        // {
        //   int a = border/2 + 1;
        //   nvgRoundedRect(ctx, mPos.x()+a, mPos.y()+a, mSize.x()-2*a, mSize.y()-2*a, mTheme->mButtonCornerRadius);
        // }
        NVGpaint img = nvgImagePattern(ctx, offset.x(), offset.y(), size.x(), size.y(), 0, mImageID,
                                       image_alpha * (mPushed ? 0.8f : 1.0f));
        nvgFillPaint(ctx, img);
        nvgFill(ctx);
    }
    nvgBeginPath(ctx);
    nvgRoundedRect(ctx, offset.x(), offset.y(), size.x() - border, size.y() - border,
                   mTheme->mButtonCornerRadius);
    if (!getDefinition()->isA("INDICATOR") && !mImageID) {
        NVGpaint bg = nvgLinearGradient(ctx, offset.x(), offset.y(), offset.x(),
                                        offset.y() + size.y(), gradTop, gradBot);
        nvgFillPaint(ctx, bg);
        nvgFill(ctx);
    }

    int fontSize = mFontSize == -1 ? mTheme->mButtonFontSize : mFontSize;
    if (mPushed)
        fontSize -= 1;
    nvgFontSize(ctx, fontSize);
    nvgFontFace(ctx, "sans-bold");

    NVGcolor textColor = mTextColor.w() == 0 ? mTheme->mTextColor : mTextColor;

    std::string text = format_string.length() > 0
                           ? format_caption(mCaption, format_string, value_type, value_scale)
                           : mCaption;
    if (mPushed) {
        if (text_on_colour.w() != 0)
            textColor = text_on_colour;
        if (!on_caption.empty())
            text = on_caption;
    }
    float tw = nvgTextBounds(ctx, 0, 0, text.c_str(), nullptr, nullptr);

    Vector2f center = mPos.cast<float>() + mSize.cast<float>() * 0.5f;
    Vector2f textPos(center.x() - tw * 0.5f, center.y() - 1);

    if (mIcon) {
        auto icon = utf8(mIcon);

        float iw, ih = fontSize;
        if (nvgIsFontIcon(mIcon)) {
            ih *= 1.5f;
            if (mIconPosition == IconPosition::Filled) {
                ih = mSize.y();
            }
            nvgFontSize(ctx, ih);
            nvgFontFace(ctx, "icons");
            iw = nvgTextBounds(ctx, 0, 0, icon.data(), nullptr, nullptr);
        }
        else {
            int w, h;
            ih *= 0.9f;
            if (mIconPosition == IconPosition::Filled) {
                ih = mSize.y();
            }
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
        }
        else if (mIconPosition == IconPosition::RightCentered) {
            textPos.x() -= iw * 0.5f;
            iconPos.x() += tw * 0.5f;
        }
        else if (mIconPosition == IconPosition::Left) {
            iconPos.x() = mPos.x() + 8;
        }
        else if (mIconPosition == IconPosition::Right) {
            iconPos.x() = mPos.x() + mSize.x() - iw - 8;
        }
        else if (mIconPosition == IconPosition::Filled) {
            iconPos.x() = mPos.x();
            iconPos.y() = mPos.y() + ih / 2; // compensates for offset below
        }

        if (nvgIsFontIcon(mIcon)) {
            nvgText(ctx, iconPos.x(), iconPos.y() + 1, icon.data(), nullptr);
        }
        else {
            NVGpaint imgPaint = nvgImagePattern(ctx, iconPos.x(), iconPos.y() - ih / 2, iw, ih, 0,
                                                mIcon, mEnabled ? 0.5f : 0.25f);

            nvgFillPaint(ctx, imgPaint);
            nvgFill(ctx);
        }
    }

    // we are using two separate alignments: 1,2,4 so we
    // convert the default alignment from nanogui here
    int align = intFromHorizontalAlignment(alignment);

    nvgFontSize(ctx, fontSize);
    nvgFontFace(ctx, "sans-bold");
    nvgTextAlign(ctx, align);
    int label_x = textPos.x();
    int label_y = textPos.y();

    if (alignment == HorizontalAlignment::Right) {
        label_x = mPos.x() + mSize.x() - 5;
    }
    else if (alignment == HorizontalAlignment::Centre) {
        label_x = wrap_text ? textPos.x() : mPos.x() + mSize.x() / 2;
    }
    else
        label_x = mPos.x();

    int alignv = 0;
    if (valign == VerticalAlignment::Top) {
        label_y = mPos.y();
        alignv = NVG_ALIGN_TOP;
        nvgTextAlign(ctx, align | alignv);
    }
    else if (valign == VerticalAlignment::Bottom) {
        alignv = NVG_ALIGN_BOTTOM;
        nvgTextAlign(ctx, align | alignv);
        if (wrap_text) {
            float bounds[4];
            nvgTextBoxBounds(ctx, mPos.x(), mPos.y(), mSize.x(), text.c_str(), nullptr, bounds);
            label_y = mPos.y() + mSize.y() - (bounds[3] - bounds[1]) / 2.0f;
        }
        else {
            label_y = mPos.y() + mSize.y();
        }
    }
    else {
        alignv = NVG_ALIGN_CENTER;
        nvgTextAlign(ctx, align | alignv);
        if (wrap_text) {
            float bounds[4];
            nvgTextBoxBounds(ctx, 0, label_y, mSize.x(), text.c_str(), nullptr, bounds);
            // Vertical centre alignment seems to need some adjustment
            auto h = bounds[3] - bounds[1];
            label_y = mPos.y() + mSize.y() / 2.0 + (1.6 * fontSize - h) / 2;
        }
        else {
            label_y += 0.35f * fontSize;
        }
    }

    if (shadow) {
        nvgFillColor(ctx, mTheme->mTextColorShadow);
        if (!wrap_text)
            nvgText(ctx, label_x, label_y, text.c_str(), nullptr);
        else {
            nvgTextBox(ctx, mPos.x(), label_y, mSize.x(), text.c_str(), nullptr);
        }
    }
    nvgFillColor(ctx, textColor);
    if (!wrap_text) {
        nvgText(ctx, label_x, label_y + 1, text.c_str(), nullptr);
    }
    else {
        nvgTextBox(ctx, mPos.x(), label_y, mSize.x(), text.c_str(), nullptr);
    }
    if (mSelected)
        drawSelectionBorder(ctx, mPos, mSize);
    else if (EDITOR->isEditMode()) {
        drawElementBorder(ctx, mPos, mSize);
    }
    nvgRestore(ctx);
}

void EditorButton::loadProperties(PropertyFormHelper *properties) {
    EditorWidget::loadProperties(properties);
    EditorButton *btn = dynamic_cast<EditorButton *>(this);
    if (btn) {
        EditorGUI *gui = EDITOR->gui();
        properties->addVariable<std::string>(
            "Off caption", [&](std::string value) mutable { setCaption(value); },
            [&]() -> std::string { return caption(); });
        properties->addVariable<std::string>(
            "On caption", [&](std::string value) mutable { setOnCaption(value); },
            [&]() -> std::string { return onCaption(); });
        properties->addVariable<std::string>(
            "Image", [&](std::string value) mutable { setImageName(value); },
            [&]() -> std::string { return imageName(); });
        properties->addVariable<float>(
            "Image opacity", [&](float value) mutable { setImageAlpha(value); },
            [&]() -> float { return imageAlpha(); });
        properties->addVariable<int>(
            "Icon", [&](int value) mutable { setIcon(value); }, [&]() -> int { return icon(); });
        properties->addVariable<HorizontalAlignment>("Alignment", alignment, true)
            ->setItems({"Left", "Centre", "Right"});
        properties->addVariable<VerticalAlignment>("Vertical Alignment", valign, true)
            ->setItems({"Top", "Centre", "Bottom"});
        properties->addVariable<bool>(
            "Wrap Text", [&](bool value) mutable { wrap_text = value; },
            [&]() -> bool { return wrap_text; });
        properties->addVariable<int>(
            "IconPosition",
            [&](int value) mutable {
                nanogui::Button::IconPosition icon_pos(nanogui::Button::IconPosition::Left);
                switch (value) {
                case 0:
                    break;
                case 1:
                    icon_pos = nanogui::Button::IconPosition::LeftCentered;
                    break;
                case 2:
                    icon_pos = nanogui::Button::IconPosition::RightCentered;
                    break;
                case 3:
                    icon_pos = nanogui::Button::IconPosition::Right;
                    break;
                case 4:
                    icon_pos = nanogui::Button::IconPosition::Filled;
                    break;
                default:
                    break;
                }
                setIconPosition(icon_pos);
            },
            [&]() -> int {
                switch (iconPosition()) {
                case nanogui::Button::IconPosition::Left:
                    return 0;
                case nanogui::Button::IconPosition::LeftCentered:
                    return 1;
                case nanogui::Button::IconPosition::RightCentered:
                    return 2;
                case nanogui::Button::IconPosition::Right:
                    return 3;
                case nanogui::Button::IconPosition::Filled:
                    return 4;
                }
                return 0;
            });
        properties->addVariable<unsigned int>(
            "Behaviour",
            [&, gui](unsigned int value) mutable {
                setFlags(value & 0xff);
                setupButtonCallbacks(remote, gui);
            },
            [&]() -> unsigned int { return flags(); });
        properties->addVariable<std::string>(
            "Command", [&](std::string value) { setCommand(value); },
            [&]() -> std::string { return command(); });
        properties->addVariable<bool>(
            "Pushed", [&](bool value) { setPushed(value); }, [&]() -> bool { return pushed(); });
        properties->addVariable<BorderStyle>("Border style", border_style, true)
            ->setItems({"None", "Shadow", "Frame"});
        properties->addVariable<BorderColouring>("Border colouring", border_colouring, true)
            ->setItems({"Manual", "Auto"});
        properties
            ->addVariable<BorderGradientDirection>("Border gradient dir", border_grad_dir, true)
            ->setItems({"Down", "Right", "Up", "Left", "TLBR", "BRTL"});
        properties->addGroup("Colours");
        properties->addVariable<nanogui::Color>(
            "Off colour",
            [&, btn](const nanogui::Color &value) mutable { setBackgroundColor(value); },
            [&, btn]() -> const nanogui::Color & { return backgroundColor(); });
        properties->addVariable<nanogui::Color>(
            "On colour", [&, btn](const nanogui::Color &value) mutable { setOnColor(value); },
            [&, btn]() -> const nanogui::Color & { return onColor(); });
        properties->addVariable<nanogui::Color>(
            "Off text colour",
            [&, btn](const nanogui::Color &value) mutable { setTextColor(value); },
            [&, btn]() -> const nanogui::Color & { return textColor(); });
        properties->addVariable<nanogui::Color>(
            "On text colour",
            [&, btn](const nanogui::Color &value) mutable { setOnTextColor(value); },
            [&, btn]() -> const nanogui::Color & { return onTextColor(); });
        properties->addVariable<nanogui::Color>(
            "Border gradient top",
            [&, btn](const nanogui::Color &value) mutable { border_grad_top = value; },
            [&, btn]() -> const nanogui::Color & { return border_grad_top; });
        properties->addVariable<nanogui::Color>(
            "Border gradient bot",
            [&, btn](const nanogui::Color &value) mutable { border_grad_bot = value; },
            [&, btn]() -> const nanogui::Color & { return border_grad_bot; });
        properties->addGroup("Remote");
        properties->addVariable<std::string>(
            "Remote object",
            [&, btn, this, properties](std::string value) {
                LinkableProperty *lp = EDITOR->gui()->findLinkableProperty(value);
                this->setRemoteName(value);
                if (this->remote)
                    this->remote->unlink(this);
                this->setRemote(lp);
                if (lp && getDefinition()->getKind() == "INDICATOR")
                    lp->link(new LinkableIndicator(this));
                //properties->refresh();
            },
            [&]() -> std::string {
                if (remote)
                    return remote->tagName();
                if (getDefinition()) {
                    const Value &rmt_v = getDefinition()->getValue("remote");
                    if (rmt_v != SymbolTable::Null)
                        return rmt_v.asString();
                }
                return "";
            });
        properties->addVariable<std::string>(
            "Connection",
            [&, this, properties](std::string value) {
                if (remote)
                    remote->setGroup(value);
                else
                    setConnection(value);
            },
            [&]() -> std::string { return remote ? remote->group() : getConnection(); });
        properties->addVariable<std::string>(
            "Visibility",
            [&, this, properties](std::string value) {
                LinkableProperty *lp = EDITOR->gui()->findLinkableProperty(value);
                if (visibility)
                    visibility->unlink(this);
                visibility = lp;
                if (lp) {
                    lp->link(new LinkableVisibility(this));
                }
            },
            [&]() -> std::string { return visibility ? visibility->tagName() : ""; });
    }
}
