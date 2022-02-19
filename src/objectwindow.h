//
//  ObjectWindow.h
//  Project: humid
//
//	All rights reserved. Use of this source code is governed by the
//	3-clause BSD License in LICENSE.txt.

#ifndef __objectwindow_h__
#define __objectwindow_h__

#include <nanogui/common.h>
#include <nanogui/screen.h>
#include <nanogui/theme.h>
#include <nanogui/widget.h>
#include <nanogui/window.h>
#include <ostream>
#include <string>

#include "editorgui.h"
#include "palette.h"
#include "skeleton.h"

/* an ObjectWindow is a palette of elements from a clockwork connection.

 The device can collect clockwork object from a tag file or from direct
 connection (TBD).
 */
class ObjectWindow : public Skeleton, public Palette {
  public:
    ObjectWindow(EditorGUI *screen, nanogui::Theme *theme, const char *tag_fname = 0);
    void setVisible(bool which) { window->setVisible(which); }
    void update();
    nanogui::Screen *getScreen() { return gui; }
    void show(nanogui::Widget &w);
    void loadTagFile(const std::string tagfn);
    void rebuildWindow();
    nanogui::Widget *createTab(const std::string tags);
    nanogui::Window *createPanelPage(const char *filename = 0,
                                     nanogui::Widget *palette_content = 0);
    bool importModbusInterface(const std::string group_name, std::istream &init,
                               nanogui::Widget *palette_content, nanogui::Widget *container);
    nanogui::Widget *getItems() { return items; }
    void loadItems(const std::string group, const std::string match);
    nanogui::Widget *getPaletteContent() { return palette_content; }

  protected:
    EditorGUI *gui;
    nanogui::Widget *items;
    nanogui::TextBox *search_box;
    nanogui::Widget *palette_content;
    nanogui::TabWidget *tab_region;
    nanogui::Widget *current_layer;
    std::map<std::string, nanogui::Widget *> layers;
};

#endif
