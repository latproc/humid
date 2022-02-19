//
//  EditorImageView.h
//  Project: humid
//
//	All rights reserved. Use of this source code is governed by the
//	3-clause BSD License in LICENSE.txt.

#ifndef __EditorImageView_h__
#define __EditorImageView_h__

#include "editorwidget.h"
#include <nanogui/imageview.h>
#include <ostream>
#include <string>

class EditorImageView : public nanogui::ImageView, public EditorWidget {

  public:
    EditorImageView(NamedObject *owner, Widget *parent, const std::string nam, LinkableProperty *lp,
                    GLuint image_id, int icon = 0);
    ~EditorImageView();

    virtual nanogui::Widget *asWidget() override { return this; }
    virtual bool mouseButtonEvent(const nanogui::Vector2i &p, int button, bool down,
                                  int modifiers) override;

    virtual bool mouseMotionEvent(const nanogui::Vector2i &p, const nanogui::Vector2i &rel,
                                  int button, int modifiers) override;

    virtual bool mouseEnterEvent(const Vector2i &p, bool enter) override;

    virtual void getPropertyNames(std::list<std::string> &names) override;
    void loadProperties(PropertyFormHelper *properties) override;
    virtual void
    loadPropertyToStructureMap(std::map<std::string, std::string> &property_map) override;
    const std::map<std::string, std::string> &property_map() const override;
    const std::map<std::string, std::string> &reverse_property_map() const override;
    virtual Value getPropertyValue(const std::string &prop) override;
    virtual void setProperty(const std::string &prop, const std::string value) override;
    virtual void draw(NVGcontext *ctx) override;

    void setImageName(const std::string new_name, bool reload = false);
    const std::string &imageName() const;

    void refresh();

  protected:
    std::string image_name;
    bool need_redraw;
};

#endif
