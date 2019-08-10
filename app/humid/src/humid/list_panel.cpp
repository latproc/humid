//
//  ScreensWindow.cpp
//  Project: humid
//
//	All rights reserved. Use of this source code is governed by the
//	3-clause BSD License in LICENSE.txt.

#include <iostream>
#include <nanogui/widget.h>
#include "list_panel.h"
#include "selectable.h"

ListPanel::ListPanel(nanogui::Widget *parent) : nanogui::Widget(parent) { 
}

void ListPanel::update() {
}

Selectable *ListPanel::getSelectedItem() {
	if (!hasSelections()) return 0;
	return *selections.begin();
}

void ListPanel::selectFirst() {
}

