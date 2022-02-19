#pragma once
#include <nanogui/theme.h>
#include <string>

#include "editorgui.h"
#include "listpanel.h"

class StartupWindow : public Skeleton {
  public:
    StartupWindow(EditorGUI *screen, nanogui::Theme *theme);
    void setVisible(bool which) { window->setVisible(which); }

  private:
    std::string message;
    EditorGUI *gui;
    ListPanel *project_box;
};
