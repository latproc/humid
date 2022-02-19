#pragma once

#include <nanogui/common.h>
#include <nanogui/formhelper.h>
#include <nanogui/screen.h>

class PropertyFormHelper : public nanogui::FormHelper {
  public:
    PropertyFormHelper(nanogui::Screen *screen, nanogui::Vector2i *fixed_size = nullptr);

    void clear();
    nanogui::Window *addWindow(const nanogui::Vector2i &pos,
                               const std::string &title = "Untitled") override;

    void setWindow(nanogui::Window *wind) override;

    nanogui::Widget *content() override;

  private:
    nanogui::Widget *mContent = nullptr;
    nanogui::Vector2i *mItemSize = nullptr;
};
