//
//  EditorCombobox.h
//  Project: humid
//
//	All rights reserved. Use of this source code is governed by the
//	3-clause BSD License in LICENSE.txt.

#pragma once

#include "editorwidget.h"
#include <nanogui/combobox.h>
#include <ostream>
#include <string>

class EditorComboBox : public nanogui::ComboBox, public EditorWidget {

  public:
    EditorComboBox(NamedObject *owner, Widget *parent, const std::string nam, LinkableProperty *lp);

    ~EditorComboBox();

    virtual nanogui::Widget *asWidget() override { return this; }
    virtual bool mouseButtonEvent(const nanogui::Vector2i &p, int button, bool down,
                                  int modifiers) override;

    virtual bool mouseMotionEvent(const nanogui::Vector2i &p, const nanogui::Vector2i &rel,
                                  int button, int modifiers) override;

    virtual bool mouseEnterEvent(const Vector2i &p, bool enter) override;

    virtual void draw(NVGcontext *ctx) override;

    virtual void getPropertyNames(std::list<std::string> &names) override;
    void loadProperties(PropertyFormHelper *properties) override;
    virtual void
    loadPropertyToStructureMap(std::map<std::string, std::string> &property_map) override;
    const std::map<std::string, std::string> &property_map() const override;
    const std::map<std::string, std::string> &reverse_property_map() const override;
    virtual Value getPropertyValue(const std::string &prop) override;
    virtual void setProperty(const std::string &prop, const std::string value) override;

    const nanogui::Color &backgroundColor() { return mBackgroundColor; }
    const nanogui::Color &textColor() { return mTextColor; }

    /// Sets the background color of this Label.
    void setBackgroundColor(const nanogui::Color &backgroundColor) {
        mBackgroundColor = backgroundColor;
    }
    void setTextColor(const nanogui::Color &textColor) { mTextColor = textColor; }

  protected:
    nanogui::Color mBackgroundColor;
    nanogui::Color mTextColor;
};
