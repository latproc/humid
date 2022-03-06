#include "thememanager.h"
#include "colourhelper.h"
#include "helper.h"
#include "structure.h"
#include <map>
#include <symboltable.h>

ThemeManager *ThemeManager::theme_manager = nullptr;

void set_prop(int &val, SymbolTable &st, const char *key) {
    const Value &v = st.find(key);
    if (!isNull(v)) {
        long iValue;
        if (v.asInteger(iValue)) {
            val = static_cast<int>(iValue);
        }
    }
}

static void set_prop(nanogui::Color &val, SymbolTable &st, const char *key) {
    const Value &v = st.find(key);
    if (!isNull(v)) {
        const auto &result = colourFromString(v.asString());
        val = result.first;
        if (!result.second.empty()) {
            std::cerr << "Error loading theme: " << result.second << "\n";
        }
    }
}

static void fromStructure(nanogui::Theme *theme, Structure *s) {
    if (!theme || !s)
        return;
    auto &props = s->getProperties();
    set_prop(theme->mStandardFontSize, props, "StandardFontSize");
    set_prop(theme->mButtonFontSize, props, "ButtonFontSize");
    set_prop(theme->mTextBoxFontSize, props, "TextBoxFontSize");
    set_prop(theme->mWindowCornerRadius, props, "WindowCornerRadius");
    set_prop(theme->mWindowHeaderHeight, props, "WindowHeaderHeight");
    set_prop(theme->mWindowDropShadowSize, props, "WindowDropShadowSize");
    set_prop(theme->mButtonCornerRadius, props, "ButtonCornerRadius");
    set_prop(theme->mDropShadow, props, "DropShadowColour");
    set_prop(theme->mTransparent, props, "TransparentColour");
    set_prop(theme->mBorderDark, props, "BorderDarkColour");
    set_prop(theme->mBorderLight, props, "BorderLightColour");
    set_prop(theme->mBorderMedium, props, "BorderMediumColour");
    set_prop(theme->mTextColor, props, "TextColour");
    set_prop(theme->mDisabledTextColor, props, "DisabledTextColour");
    set_prop(theme->mTextColorShadow, props, "TextShadowColour");
    set_prop(theme->mIconColor, props, "IconColour");
    set_prop(theme->mButtonGradientTopFocused, props, "ButtonGradientTopFocusedColour");
    set_prop(theme->mButtonGradientBotFocused, props, "ButtonGradientBottomFocusedColour");
    set_prop(theme->mButtonGradientTopUnfocused, props, "ButtonGradientTopUnfocusedColour");
    set_prop(theme->mButtonGradientBotUnfocused, props, "ButtonGradientBottomUnfocusedColour");
    set_prop(theme->mButtonGradientTopPushed, props, "ButtonGradientTopPushedColour");
    set_prop(theme->mButtonGradientBotPushed, props, "ButtonGradientBottomPushedColour");
    set_prop(theme->mWindowFillUnfocused, props, "WindowFillUnfocusedColour");
    set_prop(theme->mWindowFillFocused, props, "WindowFillFocusedColour");
    set_prop(theme->mWindowPopup, props, "WindowPopupColour");
    set_prop(theme->mWindowPopupTransparent, props, "WindowPopupTransparentColour");
}

void setupTheme(nanogui::Theme *theme) {
    using namespace nanogui;
    theme->mStandardFontSize = 20;
    theme->mButtonFontSize = 20;
    theme->mTextBoxFontSize = -1;
    theme->mWindowCornerRadius = 2;
    theme->mWindowHeaderHeight = 30;
    theme->mWindowDropShadowSize = 10;
    theme->mButtonCornerRadius = 2;
    theme->mTabBorderWidth = 0.75f;
    theme->mTabInnerMargin = 5;
    theme->mTabMinButtonWidth = 20;
    theme->mTabMaxButtonWidth = 160;
    theme->mTabControlWidth = 20;
    theme->mTabButtonHorizontalPadding = 10;
    theme->mTabButtonVerticalPadding = 2;

    theme->mDropShadow = Color(0, 128);
    theme->mTransparent = Color(0, 0);
    theme->mBorderDark = Color(29, 255);
    theme->mBorderLight = Color(92, 255);
    theme->mBorderMedium = Color(35, 255);
    theme->mTextColor = Color(0, 160);
    theme->mDisabledTextColor = Color(100, 80);
    theme->mTextColorShadow = Color(100, 160);
    theme->mIconColor = theme->mTextColor;

    theme->mButtonGradientTopFocused = Color(255, 255);
    theme->mButtonGradientBotFocused = Color(240, 255);
    theme->mButtonGradientTopUnfocused = Color(240, 255);
    theme->mButtonGradientBotUnfocused = Color(235, 255);
    theme->mButtonGradientTopPushed = Color(180, 255);
    theme->mButtonGradientBotPushed = Color(196, 255);

    /* Window-related */
    theme->mWindowFillUnfocused = Color(220, 230);
    theme->mWindowFillFocused = Color(225, 230);
    theme->mWindowTitleUnfocused = theme->mDisabledTextColor;
    theme->mWindowTitleFocused = theme->mTextColor;

    theme->mWindowHeaderGradientTop = theme->mButtonGradientTopUnfocused;
    theme->mWindowHeaderGradientBot = theme->mButtonGradientBotUnfocused;
    theme->mWindowHeaderSepTop = theme->mBorderLight;
    theme->mWindowHeaderSepBot = theme->mBorderDark;

    theme->mWindowPopup = Color(255, 255);
    theme->mWindowPopupTransparent = Color(255, 0);
}

class ThemeManager::Pimpl {
    NVGcontext *context;
    std::map<std::string, nanogui::ref<nanogui::Theme>> themes;

  public:
    void addTheme(const std::string &name, nanogui::Theme *theme) { themes[name] = theme; }
    nanogui::Theme *findTheme(const std::string &name) {
        auto found = themes.find(name);
        if (found != themes.end()) {
            return (*found).second;
        }
        return nullptr;
    }

    void setContext(NVGcontext *ctx) { context = ctx; }
    
    nanogui::Theme *createTheme() {
        assert(context);
        if (context) {
            auto theme = new nanogui::Theme(context);
            setupTheme(theme);
            return theme;
        }
        return nullptr;
    }

    nanogui::Theme *createTheme(Structure *settings) {
        auto theme = createTheme();
        fromStructure(theme, settings);
        return theme;
    }
};

ThemeManager &ThemeManager::instance() {
    if (!theme_manager)
        theme_manager = new ThemeManager;
    return *theme_manager;
}

ThemeManager::ThemeManager() { impl = new Pimpl; }
ThemeManager::~ThemeManager() { delete impl; }

void ThemeManager::setContext(NVGcontext *context) { impl->setContext(context); }

nanogui::Theme *ThemeManager::createTheme(Structure *settings) {
    return impl->createTheme(settings);
}

void ThemeManager::addTheme(const std::string &name, nanogui::Theme *theme) {
    impl->addTheme(name, theme);
}
nanogui::Theme *ThemeManager::findTheme(const std::string &name) { return impl->findTheme(name); }
