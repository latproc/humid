#pragma once

#include "editorgui.h"
#include <nanogui/theme.h>
#include <nanogui/window.h>

class Toolbar : public nanogui::Window {
  public:
    Toolbar(EditorGUI *screen, nanogui::Theme *);
    nanogui::Window *getWindow() { return this; }
    bool mouseDragEvent(const nanogui::Vector2i &p, const nanogui::Vector2i &rel, int button,
                        int modifiers) override {
        bool res = nanogui::Window::mouseDragEvent(p, rel, button, modifiers);
        updateSettingsStructure("ToolBar", this);
        return res;
    }

  private:
    EditorGUI *gui;
};
