#pragma once

#include <nanogui/vscrollpanel.h>
#include <nanogui/widget.h>

#include "palette.h"
#include "selectable.h"

class ListPanel : public nanogui::Widget, public Palette {
  public:
    ListPanel(nanogui::Widget *owner);
    virtual ~ListPanel() {}
    void update();
    Selectable *getSelectedItem();
    void selectFirst();

  private:
    nanogui::VScrollPanel *palette_scroller;
    nanogui::Widget *palette_content;
};
