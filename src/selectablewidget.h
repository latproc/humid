/*
	All rights reserved. Use of this source code is governed by the
	3-clause BSD License in LICENSE.txt.
*/

#ifndef __SelectableWidget_h__
#define __SelectableWidget_h__

#include "selectable.h"
#include "uiitem.h"
#include <nanogui/button.h>
#include <ostream>
#include <string>

class SelectableWidget : public nanogui::Widget, public Selectable, public UIItem {
  public:
    SelectableWidget(const SelectableWidget &orig);
    SelectableWidget &operator=(const SelectableWidget &other);
    std::ostream &operator<<(std::ostream &out) const;
    bool operator==(const SelectableWidget &other);

    SelectableWidget(const std::string kind, Palette *pal, nanogui::Widget *parent,
                     const std::string &caption = "Untitled");
    virtual ~SelectableWidget();
    virtual bool mouseButtonEvent(const nanogui::Vector2i &p, int button, bool down,
                                  int modifiers) override;
    virtual void draw(NVGcontext *ctx) override;

  private:
    std::string display_caption;
};

std::ostream &operator<<(std::ostream &out, const SelectableWidget &m);

#endif
