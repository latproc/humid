//
//  StructuresWindow.cpp
//  Project: humid
//
//	All rights reserved. Use of this source code is governed by the
//	3-clause BSD License in LICENSE.txt.

#include <iostream>
#include "skeleton.h"
#include "structureswindow.h"

StructuresWindow *StructuresWindow::instance_;

StructuresWindow *StructuresWindow::create(EditorGUI *screen, nanogui::Theme *theme) {
    if (!instance_) instance_ = new StructuresWindow(screen, theme);
    return instance_;
}

StructuresWindow *StructuresWindow::instance() {
    return instance_;
}

