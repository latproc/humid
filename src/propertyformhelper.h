#pragma once

#include <nanogui/screen.h>
#include <nanogui/common.h>
#include <nanogui/formhelper.h>


class PropertyFormHelper : public nanogui::FormHelper {
public:
	PropertyFormHelper(nanogui::Screen *screen) ;

	void clear();
	nanogui::Window *addWindow(const nanogui::Vector2i &pos,
							  const std::string &title = "Untitled") override;

	void setWindow(nanogui::Window *wind) override;

	nanogui::Widget *content() override;
private:
	nanogui::Widget *mContent;
};
