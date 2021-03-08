#pragma once

#include <nanogui/widget.h>
#include <nanogui/theme.h>
#include "editorgui.h"
#include "skeleton.h"

class PropertyFormWindow : public SkeletonWindow {
public:
	PropertyFormWindow(nanogui::Widget *parent, const std::string &title = "Untitled") : SkeletonWindow(parent, title), mContent(0) { }

	void setContent(nanogui::Widget *content) { mContent = content; }

	virtual bool focusEvent(bool focused) override {
		using namespace nanogui;
		return nanogui::Window::focusEvent(focused);
	}
private:
	nanogui::Widget *mContent;
};
