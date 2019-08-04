//
//  Project: humid
//
//	All rights reserved. Use of this source code is governed by the
//	3-clause BSD License in LICENSE.txt.

#ifndef list_panel_h
#define list_panel_h

#include <nanogui/common.h>
#include <nanogui/widget.h>
#include "palette.h"
#if defined(_WIN32)
#include <windows.h>
#endif

class Selectable;
class EditorGUI;
class ListPanel : public nanogui::Widget, public Palette {
public:
	ListPanel(nanogui::Widget *owner);
	virtual ~ListPanel() {}
	void update();
	Selectable *getSelectedItem();
	void selectFirst();

private:
	nanogui::VScrollPanel *palette_scroller;
	nanogui::Widget *palette_content;
};

#endif /* list_panel_h */
