/*
	All rights reserved. Use of this source code is governed by the
	3-clause BSD License in LICENSE.txt.
*/

#ifndef __SelectableButton_h__
#define __SelectableButton_h__

#include "selectable.h"
#include "uiitem.h"
#include <nanogui/button.h>
#include <ostream>
#include <string>

class SelectableButton : public nanogui::Button, public Selectable, public UIItem {
  public:
    SelectableButton(const SelectableButton &orig);
    SelectableButton &operator=(const SelectableButton &other);
    std::ostream &operator<<(std::ostream &out) const;
    bool operator==(const SelectableButton &other);

    SelectableButton(const std::string kind, Palette *pal, nanogui::Widget *parent,
                     const std::string &caption = "Untitled");
    bool selected() { return mSelected; }

    virtual bool mouseButtonEvent(const nanogui::Vector2i &p, int button, bool down,
                                  int modifiers) override;
    virtual void draw(NVGcontext *ctx) override;
    virtual nanogui::Widget *create(nanogui::Widget *container) const;

    void setPassThrough(bool which) { pass_through = which; }

  private:
    std::string display_caption;
    bool pass_through; // pass mouse events through to button handlers
};

std::ostream &operator<<(std::ostream &out, const SelectableButton &m);

#endif
