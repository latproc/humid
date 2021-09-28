#pragma once
#include <string>
#include <nanogui/object.h>
#include <nanogui/theme.h>
#include <nanogui/common.h>

class Structure;
class ThemeManager {
public:
    static ThemeManager & instance();

    void addTheme(const std::string &name, nanogui::Theme *theme);
    nanogui::Theme *findTheme(const std::string &name);

    // A context to create the theme in.
    void setContext(NVGcontext *context);

    // Construct a theme from a given structure definition.
    nanogui::Theme *createTheme(Structure *settings = nullptr);

private:
    class Pimpl;
    Pimpl *impl = nullptr;
    ThemeManager();
    ~ThemeManager();
    static ThemeManager *theme_manager;
};
