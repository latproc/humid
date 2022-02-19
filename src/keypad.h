#pragma once

#include "dialogwindow.h"
#include "editortextbox.h"

class Keypad : public DialogWindow {
    Keypad(EditorGUI *screen, nanogui::Theme *theme) : DialogWindow(screen, theme) {}
    void set_target(EditorTextBox *);

    // display the window with the target value preloaded
    bool show(const std::string &dialog_name);

  private:
    EditorTextBox *target;
};
