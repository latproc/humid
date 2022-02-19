#pragma once

#include <nanogui/common.h>
#include <nanogui/glutil.h>
#include <nanogui/opengl.h>
#include <string>

#include "editorgui.h"
#include "editorobject.h"
#include "skeleton.h"

class UserWindowWin : public SkeletonWindow, public EditorObject {
  public:
    UserWindowWin(EditorGUI *s, const std::string caption)
        : SkeletonWindow(s, caption), EditorObject(0), gui(s), current_item(-1) {}

    bool keyboardEvent(int key, int /* scancode */, int action, int modifiers) override;

    bool mouseEnterEvent(const nanogui::Vector2i &p, bool enter) override;

    virtual bool mouseButtonEvent(const nanogui::Vector2i &p, int button, bool down,
                                  int modifiers) override {
        if (button == GLFW_MOUSE_BUTTON_RIGHT && !down)
            return true;
        return SkeletonWindow::mouseButtonEvent(p, button, down, modifiers);
    }

    void update();

    virtual void draw(NVGcontext *ctx) override;

    void setCurrentItem(int n) { current_item = n; }
    int currentItem() { return current_item; }

  private:
    EditorGUI *gui;
    int current_item;
};
