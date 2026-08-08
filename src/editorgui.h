/*
	All rights reserved. Use of this source code is governed by the
	3-clause BSD License in LICENSE.txt.
*/

#ifndef __EditorGUI_h__
#define __EditorGUI_h__

#include <ostream>
#include <string>
#include <chrono>
#include <nanogui/common.h>
#include <nanogui/widget.h>
#include <nanogui/window.h>
#include <nanogui/screen.h>
#include <nanogui/opengl.h>

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
#include "dialogwindow.h"
#include "cJSON.h"
#include "viewlistcontroller.h"

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
	DialogWindow *getUserDialog();
	void setUserDialog(const std::string &dialog_name);
	void showDialog(bool show = true);
	// Cancel NanoGUI drag before destroying widgets that may be mDragWidget
	// (e.g. screen change or dialog hide mid-press). Prevents use-after-free
	// on the subsequent mouse-release/drag events.
	void cancelActiveDrag();
	nanogui::Window *getActiveWindow();

	nanogui::Window *getNamedWindow(const std::string name);

	void *setActiveWindow(nanogui::Window*);
	void createStructures(const nanogui::Vector2i &p, std::set<Selectable *> selections);

	virtual bool mouseButtonEvent(const nanogui::Vector2i &p, int button, bool down, int modifiers) override;
	virtual bool resizeEvent(const nanogui::Vector2i &) override;
	virtual bool keyboardEvent(int key, int scancode , int action, int modifiers) override;

	GLuint getImageId(const char *, bool reload = false);
	void freeImage(GLuint image_id);
	void update(ClockworkClient::Connection *connection, bool allow_data_sync) override;

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
	void configureCapture(const std::string &path, const std::string &screen_name, int timeout_seconds);
	bool shouldIgnoreRemoteScreen() const { return capture_enabled; }
	bool captureTimedOut() const { return capture_timed_out; }

private:
	void afterFrameRendered() override;
	bool connectionsReadyForCapture();
	bool activeScreenReadyForCapture();
	size_t expectedCaptureConnectionCount();
	bool captureDeadlineExceeded() const;
	void tryCaptureFrame();

	static Structure *system_settings;
	std::recursive_mutex linkables_mutex;
	std::map<std::string, LinkableProperty*>linkables;
	ViewListController views;
	std::list<PanelScreen*>user_screens;
	nanogui::Vector2i old_size;
	nanogui::Theme *theme = nullptr;
	GuiState state;
	Editor *editor = nullptr;
	Toolbar *w_toolbar = nullptr;
	PropertyWindow *w_properties = nullptr;
	ObjectWindow *w_objects = nullptr;
	ThemeWindow *w_theme = nullptr;
	UserWindow *w_user = nullptr;
	DialogWindow *w_dialog = nullptr;
	PatternsWindow *w_patterns = nullptr;
	StructuresWindow *w_structures = nullptr;
	PatternsWindow *w_connections = nullptr;
	StartupWindow *w_startup = nullptr;
	ScreensWindow *w_screens = nullptr;
	ViewsWindow *w_views;
	using imagesDataType = std::vector<std::pair<GLTexture, GLTexture::handleType>>;
	imagesDataType mImagesData;
	bool needs_update;
	unsigned int sample_buffer_size;
	EditorProject *project = nullptr;
  std::string dialog_name;
	std::string capture_path;
	std::string capture_screen_name;
	std::chrono::steady_clock::time_point capture_started_at;
	int capture_timeout_seconds = 60;
	int capture_frames_remaining = -1;
	bool capture_enabled = false;
	bool capture_written = false;
	bool capture_timed_out = false;
};

std::ostream &operator<<(std::ostream &out, const EditorGUI &m);

#endif
