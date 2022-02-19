//
//  EditorButton.h
//  Project: humid
//
//	All rights reserved. Use of this source code is governed by the
//	3-clause BSD License in LICENSE.txt.

#ifndef __EditorButton_h__
#define __EditorButton_h__

#include <map>
#include <ostream>
#include <string>

#include "editorwidget.h"
#include <nanogui/button.h>
#include <nanogui/common.h>
#include <nanogui/widget.h>

using nanogui::Widget;
class EditorGUI;

class EditorButton : public nanogui::Button, public EditorWidget {

  public:
    enum class HorizontalAlignment { Left, Centre, Right };
    enum class VerticalAlignment { Top, Centre, Bottom };
    enum class BorderStyle { None, Shadow, Frame };
    enum class BorderColouring { Manual, Auto };
    enum class BorderGradientDirection { Down, Right, Up, Left, TLBR, BRTL };

    EditorButton(NamedObject *owner, Widget *parent, const std::string &btn_name,
                 LinkableProperty *lp, const std::string &caption = "Untitled", bool toggle = false,
                 int icon = 0);
    ~EditorButton();

    virtual nanogui::Widget *asWidget() override { return this; }
    virtual void getPropertyNames(std::list<std::string> &names) override;
    void loadProperties(PropertyFormHelper *properties) override;
    virtual void
    loadPropertyToStructureMap(std::map<std::string, std::string> &property_map) override;
    const std::map<std::string, std::string> &property_map() const override;
    const std::map<std::string, std::string> &reverse_property_map() const override;

    virtual Value getPropertyValue(const std::string &prop) override;
    virtual void setProperty(const std::string &prop, const std::string value) override;

    virtual bool mouseButtonEvent(const nanogui::Vector2i &p, int button, bool down,
                                  int modifiers) override;

    virtual bool mouseMotionEvent(const nanogui::Vector2i &p, const nanogui::Vector2i &rel,
                                  int button, int modifiers) override;

    virtual bool mouseEnterEvent(const Vector2i &p, bool enter) override;
    void setCommand(std::string cmd);
    const std::string &command() const;

    void setupButtonCallbacks(LinkableProperty *lp, EditorGUI *gui);

    void setOnColor(nanogui::Color c) { bg_on_color = c; }
    nanogui::Color &onColor() { return bg_on_color; }
    void setOnTextColor(nanogui::Color c) { text_on_colour = c; }
    nanogui::Color &onTextColor() { return text_on_colour; }

    void setOnCaption(const std::string c) { on_caption = c; }
    const std::string &onCaption() { return on_caption; }

    void setBorderStyle(BorderStyle s) { border_style = s; }
    void setBorderGradientDir(BorderGradientDirection d) { border_grad_dir = d; }
    void setBorderColouring(BorderColouring c) { border_colouring = c; }
    void setBorderGradTop(const nanogui::Color &c) { border_grad_top = c; }
    void setBorderGradBot(const nanogui::Color &c) { border_grad_bot = c; }

    virtual void draw(NVGcontext *ctx) override;

    void setImageName(const std::string &name);
    std::string imageName() const;

    void setImageAlpha(float alpha) { image_alpha = alpha; }
    float imageAlpha() { return image_alpha; }

    void setWrap(bool which) { wrap_text = which; }

  protected:
    bool is_toggle;
    std::string command_str;
    nanogui::Color bg_on_color;
    nanogui::Color text_on_colour;
    std::string on_caption;
    HorizontalAlignment alignment = HorizontalAlignment::Centre;
    VerticalAlignment valign = VerticalAlignment::Centre;
    bool wrap_text = false;
    int shadow = 1;
    Value image_name;
    int mImageID = 0;
    float image_alpha = 1.0f;
    BorderColouring border_colouring = BorderColouring::Manual;
    BorderGradientDirection border_grad_dir = BorderGradientDirection::Down;
    nanogui::Color border_grad_bot = {51, 255};
    nanogui::Color border_grad_top = {204, 255};
    BorderStyle border_style = BorderStyle::Shadow;
};

#endif
