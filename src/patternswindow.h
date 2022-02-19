//
//  PatternsWindow.h
//  Project: humid
//
//	All rights reserved. Use of this source code is governed by the
//	3-clause BSD License in LICENSE.txt.

#ifndef __patternswindow_h__
#define __patternswindow_h__

#include <nanogui/common.h>
#include <nanogui/screen.h>
#include <nanogui/theme.h>
#include <nanogui/widget.h>
#include <nanogui/window.h>
#include <ostream>
#include <string>

#include "editorgui.h"
#include "palette.h"
#include "skeleton.h"

class PatternsWindow : public Skeleton, public Palette {
  public:
    PatternsWindow(EditorGUI *screen, nanogui::Theme *theme);
    void setVisible(bool which) { window->setVisible(which); }

  private:
    EditorGUI *gui;
};

#endif
