/*
	All rights reserved. Use of this source code is governed by the
	3-clause BSD License in LICENSE.txt.
*/

#ifndef __EditorGUI_h__
#define __EditorGUI_h__

#include <ostream>
#include <string>
#include <nanogui/common.h>
#include <nanogui/widget.h>
#include <nanogui/window.h>
#include <nanogui/screen.h>
#include <nanogui/opengl.h>
#ifdef _WIN32
#include <Win32Helper.h>
#else
#include <mutex>
#endif

#include "gltexture.h"

#include "skeleton.h"
#include "selectable.h"
#include "palette.h"
#include "structure.h"
#include "userwindow.h"
#include "editorproject.h"
#include "editorsettings.h"
#include "propertywindow.h"
#include "themewindow.h"

class StartupWindow;
class PropertyWindow;
class ObjectWindow;
class ThemeWindow;
class Toolbar;
class Editor;
class StructuresWindow;
class PatternsWindow;
class UIStructure;
class PropertyFormHelper;
class ScreensWindow;
class ViewsWindow;

class EditorGUI : public ClockworkClient {
public:
	enum GuiState { GUIWELCOME, GUISELECTPROJECT, GUICREATEPROJECT, GUIWORKING, GUIEDITMODE };
	EditorGUI(int width = 1024, int height = 768, bool full_screen = false);

	virtual void moveWindowToFront(nanogui::Window *window) override { if (!w_user || window != w_user->getWindow() ) Screen::moveWindowToFront(window); }

	void setTheme(nanogui::Theme *theme) override;
	void createWindows();

	GuiState getState() { return state; }
	void setState(GuiState s);
	void nextState();

	StartupWindow *getStartupWindow();
	PropertyWindow *getPropertyWindow();
	ObjectWindow *getObjectWindow();
	ThemeWindow *getThemeWindow();
	UserWindow *getUserWindow();
	Toolbar *getToolbar();
	StructuresWindow *getStructuresWindow();
	PatternsWindow *getPatternsWindow();
	ScreensWindow *getScreensWindow();
	ViewsWindow *getViewsWindow();
	nanogui::Window *getActiveWindow();

	nanogui::Window *getNamedWindow(const std::string name);

	void *setActiveWindow(nanogui::Window*);
	void createStructures(const nanogui::Vector2i &p, std::set<Selectable *> selections);

	virtual bool mouseButtonEvent(const nanogui::Vector2i &p, int button, bool down, int modifiers) override;
	virtual bool resizeEvent(const nanogui::Vector2i &) override;
	virtual bool keyboardEvent(int key, int scancode , int action, int modifiers) override;

	GLuint getImageId(const char *, bool reload = false);
	void update(ClockworkClient::Connection *connection) override;

	void handleRawMessage(unsigned long time, void *data) override {};
	virtual void handleClockworkMessage(ClockworkClient::Connection *conn,
		unsigned long time, const std::string &op, std::list<Value> *message) override;

	void needsUpdate() { needs_update = true; }

	unsigned int sampleBufferSize() const { return sample_buffer_size; }

	static Structure *systemSettings() { return system_settings; }
	static void systemSettings(Structure *s) { system_settings = s; }
	Structure *getSettings();
	void updateProperties() {
		EditorSettings::flush();
	}
	ViewListController &getViewManager() { return views; }

	std::list<PanelScreen*> &getScreens() { return user_screens; }
	bool changeName(EditorObject*, const std::string &oldname, const std::string &newname);

	int getSampleBufferSize() { return sample_buffer_size; }
	LinkableProperty *findLinkableProperty(const std::string name);
	void addLinkableProperty(const std::string name, LinkableProperty*lp);
	std::map<std::string, LinkableProperty*>getLinkableProperties() {
		return linkables;
	}
	void processModbusInitialisation(const std::string group_name, cJSON *obj);

private:
	static Structure *system_settings;
    #ifdef MINGW_USE_BOOST_MUTEX
    	boost::recursive_mutex linkables_mutex;
    #else
        std::recursive_mutex linkables_mutex;
    #endif
	std::map<std::string, LinkableProperty*>linkables;
	ViewListController views;
	std::list<PanelScreen*>user_screens;
	nanogui::Vector2i old_size;
	nanogui::Theme *theme;
	GuiState state;
	Editor *editor;
	Toolbar *w_toolbar;
	PropertyWindow *w_properties;
	ObjectWindow *w_objects;
	ThemeWindow *w_theme;
	UserWindow *w_user;
	PatternsWindow *w_patterns;
	StructuresWindow *w_structures;
	PatternsWindow *w_connections;
	StartupWindow *w_startup;
	ScreensWindow *w_screens;
	ViewsWindow *w_views;
	using imagesDataType = std::vector<std::pair<GLTexture, GLTexture::handleType>>;
	imagesDataType mImagesData;
	bool needs_update;
	unsigned int sample_buffer_size;
	EditorProject *project;
};

std::ostream &operator<<(std::ostream &out, const EditorGUI &m);

#endif
