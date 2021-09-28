#include <map>
#include "thememanager.h"
#include "structure.h"
#include <symboltable.h>
#include "structure.h"

ThemeManager *ThemeManager::theme_manager = nullptr;

void setupTheme(nanogui::Theme *theme) {
	using namespace nanogui;
	theme->mStandardFontSize                 = 20;
	theme->mButtonFontSize                   = 20;
	theme->mTextBoxFontSize                  = -1;
	theme->mWindowCornerRadius               = 2;
	theme->mWindowHeaderHeight               = 30;
	theme->mWindowDropShadowSize             = 10;
	theme->mButtonCornerRadius               = 2;
	theme->mTabBorderWidth                   = 0.75f;
	theme->mTabInnerMargin                   = 5;
	theme->mTabMinButtonWidth                = 20;
	theme->mTabMaxButtonWidth                = 160;
	theme->mTabControlWidth                  = 20;
	theme->mTabButtonHorizontalPadding       = 10;
	theme->mTabButtonVerticalPadding         = 2;

	theme->mDropShadow                       = Color(0, 128);
	theme->mTransparent                      = Color(0, 0);
	theme->mBorderDark                       = Color(29, 255);
	theme->mBorderLight                      = Color(92, 255);
	theme->mBorderMedium                     = Color(35, 255);
	theme->mTextColor                        = Color(0, 160);
	theme->mDisabledTextColor                = Color(100, 80);
	theme->mTextColorShadow                  = Color(100, 160);
	theme->mIconColor                        = theme->mTextColor;

	theme->mButtonGradientTopFocused         = Color(255, 255);
	theme->mButtonGradientBotFocused         = Color(240, 255);
	theme->mButtonGradientTopUnfocused       = Color(240, 255);
	theme->mButtonGradientBotUnfocused       = Color(235, 255);
	theme->mButtonGradientTopPushed          = Color(180, 255);
	theme->mButtonGradientBotPushed          = Color(196, 255);

	/* Window-related */
	theme->mWindowFillUnfocused              = Color(220, 230);
	theme->mWindowFillFocused                = Color(225, 230);
	theme->mWindowTitleUnfocused             = theme->mDisabledTextColor;
	theme->mWindowTitleFocused               = theme->mTextColor;

	theme->mWindowHeaderGradientTop          = theme->mButtonGradientTopUnfocused;
	theme->mWindowHeaderGradientBot          = theme->mButtonGradientBotUnfocused;
	theme->mWindowHeaderSepTop               = theme->mBorderLight;
	theme->mWindowHeaderSepBot               = theme->mBorderDark;

	theme->mWindowPopup                      = Color(255, 255);
	theme->mWindowPopupTransparent           = Color(255, 0);
}

class ThemeManager::Pimpl {
    NVGcontext *context;
    std::map<std::string, nanogui::ref<nanogui::Theme>> themes;
public:
    void addTheme(const std::string &name, nanogui::Theme *theme) {
        themes[name] = theme;
    }
    nanogui::Theme *findTheme(const std::string &name) {
        auto found = themes.find(name);
        if (found!= themes.end()) { return (*found).second; }
        return nullptr;
    }

    void setContext(NVGcontext *ctx) { context = ctx; }

    nanogui::Theme *createTheme(Structure *settings) {
        assert(context);
        if (context) {
            auto theme = new nanogui::Theme(context);
            setupTheme(theme);
            // TODO: copy user provided properties into the theme
            return theme;
        }
        return nullptr;
    }

};

ThemeManager &ThemeManager::instance() {
    if (!theme_manager) theme_manager = new ThemeManager;
    return *theme_manager;
}

ThemeManager::ThemeManager() { impl = new Pimpl; }
ThemeManager::~ThemeManager() {delete impl; }

void ThemeManager::setContext(NVGcontext *context) {
    impl->setContext(context);
}

nanogui::Theme *ThemeManager::createTheme(Structure *settings) {
    return impl->createTheme(settings);
}

void ThemeManager::addTheme(const std::string &name, nanogui::Theme *theme) {
    impl->addTheme(name, theme);
}
nanogui::Theme *ThemeManager::findTheme(const std::string &name) {
    return impl->findTheme(name);
}
