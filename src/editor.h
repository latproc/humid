//
//  Editor.h
//  Project: humid
//
//	All rights reserved. Use of this source code is governed by the
//	3-clause BSD License in LICENSE.txt.

#ifndef __Editor_h__
#define __Editor_h__

#include <ostream>
#include <string>
#include "editorgui.h"
#include "draghandle.h"

class Editor {

public:
	Editor(EditorGUI *gui);
	static Editor *instance();
	void setEditMode(bool which);
	bool isEditMode() ;
	bool isCreateMode();

	void setCreateMode(bool which);
	void refresh(bool );
	nanogui::DragHandle *getDragHandle();

	void saveAs(const std::string &path);
	void load(const std::string &path);
	void save();

	EditorGUI *gui() { return screen; }

private:
	static Editor *_instance;
	bool mEditMode;
	bool mCreateMode;
	EditorGUI *screen;
	nanogui::DragHandle *drag_handle;
};

#define EDITOR Editor::instance()

#endif
