//
//  ThemeWindow.cpp
//  Project: humid
//
//	All rights reserved. Use of this source code is governed by the
//	3-clause BSD License in LICENSE.txt.

#include <iostream>

#include <nanogui/theme.h>
#include <nanogui/window.h>

#include "themewindow.h"
#include "editorgui.h"
#include "propertyformhelper.h"

std::map<std::string, std::string> theme_properties;

void setup(Structure *s) {
	auto & prop = theme_properties;
    prop["Standard Font Size"] = "StandardFontSize";
    prop["Button Font Size"] = "ButtonFontSize";
    prop["TextBox Font Size"] = "TextBoxFontSize";
    prop["Corner Radius"] = "CornerRadius";
    prop["Header Height"] = "HeaderHeight";
    prop["Drop Shadow Size"] = "DropShadowSize";
    prop["Button Corner Radius"] = "ButtonCornerRadius";
    prop["Drop Shadow Colour"] = "DropShadowColour";
    prop["Transparent Colour"] = "TransparentColour";
    prop["Dark Border Colour"] = "DarkBorderColour";
    prop["Light Border Colour"] = "LightBorderColour";
    prop["Medium Border Colour"] = "MediumBorderColour";
    prop["Text Colour"] = "TextColour";
    prop["Disabled Text Colour"] = "DisabledTextColour";
    prop["Text Shadow Colour"] = "TextShadowColour";
    prop["Icon Colour"] = "IconColour";
    prop["Focussed Btn Gradient Top Colour"] = "FocussedBtnGradientTopColour";
    prop["Focussed Btn Bottom Colour"] = "FocussedBtnBottomColour";
    prop["Btn Gradient Top Colour"] = "BtnGradientTopColour";
    prop["Btn Gradient Bottom Colour"] = "BtnGradientBottomColour";
    prop["Pushed Btn Top Colour"] = "PushedBtnTopColour";
    prop["Pushed Btn Bottom Colour"] = "PushedBtnBottomColour";
    prop["Window Colour"] = "WindowColour";
    prop["Focussed Win Colour"] = "FocussedWinColour";
    prop["Window Title Colour"] = "WindowTitleColour";
    prop["Focussed Win Title Colour"] = "FocussedWinTitleColour";
}

ThemeWindow::ThemeWindow(EditorGUI *screen, nanogui::Theme *theme, nanogui::Theme *user_theme) : gui(screen), main_theme(user_theme) {
	using namespace nanogui;
	properties = new PropertyFormHelper(screen);
	window = properties->addWindow(nanogui::Vector2i(80, 50), "Theme Properties");
	window->setTheme(theme);
	loadTheme(user_theme);
	window->setVisible(false);
}

void ThemeWindow::loadTheme(nanogui::Theme *theme) {
	nanogui::Window *pw =getWindow();
	if (!pw) return;
	EditorGUI *app = gui;

	properties->clear();
	properties->setWindow(pw); // reset the grid layout
	properties->addVariable("Standard Font Size", theme->mStandardFontSize);
	properties->addVariable("Button Font Size", theme->mButtonFontSize);
	//properties->addVariable("TextBox Font Size", theme->mTextBoxFontSize);
	properties->addVariable<int> ("TextBox Font Size",
									  [theme, app](int value) {
										  theme->mTextBoxFontSize = value;
										  app->getUserWindow()->getWindow()->performLayout( app->nvgContext() );
									   },
									  [theme]()->int{ return theme->mTextBoxFontSize; });
	properties->addVariable("Corner Radius", theme->mWindowCornerRadius);
	properties->addVariable("Header Height", theme->mWindowHeaderHeight);
	properties->addVariable("Drop Shadow Size", theme->mWindowDropShadowSize);
	properties->addVariable("Button Corner Radius", theme->mButtonCornerRadius);
	properties->addVariable("Drop Shadow Colour", theme->mDropShadow);
	properties->addVariable("Transparent Colour", theme->mTransparent);
	properties->addVariable("Dark Border Colour", theme->mBorderDark);
	properties->addVariable("Light Border Colour", theme->mBorderLight);
	properties->addVariable("Medium Border Colour", theme->mBorderMedium);
	properties->addVariable("Text Colour", theme->mTextColor);
	properties->addVariable("Disabled Text Colour", theme->mDisabledTextColor);
	properties->addVariable("Text Shadow Colour", theme->mTextColorShadow);
	properties->addVariable("Icon Colour", theme->mIconColor);
	properties->addVariable("Focussed Btn Gradient Top Colour", theme->mButtonGradientTopFocused);
	properties->addVariable("Focussed Btn Bottom Colour", theme->mButtonGradientBotFocused);
	properties->addVariable("Btn Gradient Top Colour", theme->mButtonGradientTopUnfocused);
	properties->addVariable("Btn Gradient Bottom Colour", theme->mButtonGradientBotUnfocused);
	properties->addVariable("Pushed Btn Top Colour", theme->mButtonGradientTopPushed);
	properties->addVariable("Pushed Btn Bottom Colour", theme->mButtonGradientBotPushed);
	properties->addVariable("Window Colour", theme->mWindowFillUnfocused);
	properties->addVariable("Focussed Win Colour", theme->mWindowFillFocused);
	properties->addVariable("Window Title Colour", theme->mWindowTitleUnfocused);
	properties->addVariable("Focussed Win Title Colour", theme->mWindowTitleFocused);
	gui->performLayout();
}

