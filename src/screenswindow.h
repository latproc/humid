//
//  ScreensWindow.h
//  Project: humid
//
//	All rights reserved. Use of this source code is governed by the
//	3-clause BSD License in LICENSE.txt.

#ifndef __ScreensWindow_h__
#define __ScreensWindow_h__

#include "palette.h"
#include "skeleton.h"
#include <nanogui/vscrollpanel.h>
#include <nanogui/widget.h>
#include <ostream>
#include <string>

class EditorGUI;
class UserWindow;
class Structure;
class Selectable;

class ScreensWindow : public Skeleton, public Palette {
  public:
    ScreensWindow(EditorGUI *screen, nanogui::Theme *theme);
    void setVisible(bool which) { window->setVisible(which); }
    void update();
    UserWindow *getUserWindow();
    virtual void clearSelections(Selectable *except = 0) override;
    Structure *getSelectedStructure();
    void selectFirst();
    void select(const std::string screen_name);
    void updateSelectedName();

  private:
    EditorGUI *gui;
    nanogui::VScrollPanel *palette_scroller;
    nanogui::Widget *palette_content;
};

#endif
