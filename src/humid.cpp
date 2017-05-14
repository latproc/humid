/*
	All rights reserved. Use of this source code is governed by the
	3-clause BSD License in LICENSE.txt.
*/
/* This file is formatted with:
    astyle --style=kr --indent=tab=2 --one-line=keep-blocks --brackets=break-closing
*/
#include <nanogui/screen.h>
#include <nanogui/window.h>
#include <nanogui/layout.h>
#include <nanogui/label.h>
#include <nanogui/checkbox.h>
#include <nanogui/button.h>
#include <nanogui/toolbutton.h>
#include <nanogui/popupbutton.h>
#include <nanogui/combobox.h>
#include <nanogui/progressbar.h>
#include <nanogui/entypo.h>
#include <nanogui/messagedialog.h>
#include <nanogui/textbox.h>
#include <nanogui/slider.h>
#include <nanogui/imagepanel.h>
#include <nanogui/imageview.h>
#include <nanogui/vscrollpanel.h>
#include <nanogui/colorwheel.h>
#include <nanogui/graph.h>
#include <nanogui/formhelper.h>
#include <nanogui/theme.h>
#include <nanogui/tabwidget.h>
#include <nanogui/common.h>
#if defined(_WIN32)
#include <windows.h>
#endif
#include <nanogui/opengl.h>
#include <nanogui/glutil.h>
#define NANOVG_GL3_IMPLEMENTATION
#include <nanovg_gl.h>

#include <iostream>
#include <fstream>
#include <locale>
#include <string>
#include <vector>
#include "PropertyMonitor.h"
#include "DragHandle.h"
#include "manuallayout.h"
#include "skeleton.h"
#include "lineplot.h"
#include "PanelScreen.h"

#include <libgen.h>
#include <zmq.hpp>
#include <cJSON.h>
#include <MessageEncoding.h>
#include <MessagingInterface.h>
#include <signal.h>
#include <SocketMonitor.h>
#include <ConnectionManager.h>
#include <circularbuffer.h>

#include <boost/thread.hpp>
#include <boost/thread/condition.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/program_options.hpp>
#include "PropertyList.h"
#include "GLTexture.h"
#include "Palette.h"
#include "SelectableWidget.h"
#include "SelectableButton.h"
#include "UserWindow.h"

// settings file parser globals
#define __MAIN__ 1
#include "settingslang.h"
SymbolTable globals;
std::list<Structure *>st_structures;
extern FILE *st_yyin;
int st_yyparse();
extern int st_yycharno;
extern int st_yylineno;
const char *st_yyfilename = 0;
const char *yyfilename = 0;

int num_errors = 0;
std::list<std::string>error_messages;
std::list<std::string>settings_files;



using std::cout;
using std::cerr;
using std::endl;
using std::locale;
using nanogui::Vector2i;
using nanogui::Vector2f;
using Eigen::Vector2d;
using Eigen::MatrixXd;
using Eigen::Matrix3d;

namespace po = boost::program_options;

extern const int DEBUG_ALL;
#define DEBUG_BASIC ( 1 & debug)
extern int debug;
extern int saved_debug;

const char *program_name;

extern int cw_out;
extern std::string host;
extern const char *local_commands;
extern ProgramState program_state;
extern struct timeval start;

extern void setup_signals();

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
class SeriesWindow;

Handle::Mode all_handles[] = {
	Handle::POSITION,
	Handle::RESIZE_TL,
	Handle::RESIZE_T,
	Handle::RESIZE_TR,
	Handle::RESIZE_R,
	Handle::RESIZE_BL,
	Handle::RESIZE_L,
	Handle::RESIZE_BR,
	Handle::RESIZE_B
};

class SettingsItem {
public:
	SettingsItem(nanogui::Widget *w) : widget(w) {}
	bool load(Structure *item) {
		if (widget) {
			nanogui::Screen *screen = dynamic_cast<nanogui::Screen*>(widget);
			const Value &vx = item->getProperties().find("x");
			const Value &vy = item->getProperties().find("y");
			long x, y;
			if (vx.asInteger(x) && vy.asInteger(y)) {
				if (screen)
					screen->setPosition(nanogui::Vector2i(x, y));
				else
					widget->setPosition(nanogui::Vector2i(x, y));
			}
			const Value &vw = item->getProperties().find("w");
			const Value &vh = item->getProperties().find("h");
			long w, h;
			if (vw.asInteger(w) && vh.asInteger(h)) {
				if (screen) {
					screen->setSize(nanogui::Vector2i(w, h));
				}
				else {
					widget->setSize(nanogui::Vector2i(w, h));
					widget->setFixedSize(nanogui::Vector2i(w, h));
				}
			}
			return true;
		}
		else return false;
	}
	bool save(std::ostream &out, const std::string &name) {
		if (widget) {
			nanogui::Vector2i pos = widget->position();
			nanogui::Vector2i siz = widget->size();
			out << name << " WINDOW " << "("
			<< "x:" << pos.x() << ",y:" << pos.y()
			<< ",w:" << siz.x() << ",h:" << siz.y()
			<< ");\n";
			return true;
		}
		return false;
	}
private:
	nanogui::Widget *widget;
};

class EditorSettings {
public:
	void load(const std::string object_name, nanogui::Widget *widget) {
		for (auto item : st_structures) {
			if (item->getName() == object_name) {
				SettingsItem(widget).load(item);
			}
		}
	}
	void save() {
		if (settings_files.size() == 0) return;
		std::string fname(settings_files.front());
		std::ofstream settings(fname);
		for (auto w : widgets) {
			SettingsItem(w.second).save(settings, w.first);
		}
		settings.close();
	}

	void add(const std::string &name, nanogui::Widget *w) {
		widgets[name] = w;
	}

private:
	std::map<std::string, nanogui::Widget *> widgets;
};

class ViewOptions {
public:
	bool visible;
};

class ViewManager {
public:
	void set(nanogui::Widget *w, bool vis) {
		items[w].visible = vis;
		ViewOptions vo = items[w];
		int x =1;
	}
	ViewOptions get(nanogui::Widget *w) {
		// default is for views to be visible
		if (items.find(w) == items.end())
			set(w, true);
		return items[w];
	}
	void remove(nanogui::Widget *w) { items.erase(w); }

private:
	std::map<nanogui::Widget*, ViewOptions> items;
};

class EditorGUI : public ClockworkClient {
public:
	enum GuiState { GUIWELCOME, GUISELECTPROJECT, GUICREATEPROJECT, GUIWORKING, GUIEDITMODE };
	EditorGUI();

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
	SeriesWindow *getSeriesWindow();
	nanogui::Window *getActiveWindow();
	void *setActiveWindow(nanogui::Window*);
	void createStructures(const nanogui::Vector2i &p, std::set<Selectable *> selections);

	virtual bool mouseButtonEvent(const nanogui::Vector2i &p, int button, bool down, int modifiers) override;
	virtual bool resizeEvent(const Vector2i &) override;
	virtual bool keyboardEvent(int key, int scancode , int action, int modifiers) override;

	GLuint getImageId(const char *); 
	void update() override;

	void handleRawMessage(unsigned long time, void *data) override {};
	void handleClockworkMessage(unsigned long time, const std::string &op, std::list<Value> *message) override;

	void needsUpdate() { needs_update = true; }

	unsigned int sampleBufferSize() const { return sample_buffer_size; }

	void updateSettings(nanogui::Widget *source) {
		settings.save();
	}
	ViewManager &getViewManager() { return views; }

	std::list<PanelScreen*> &getScreens() { return user_screens; }

private:
	ViewManager views;
	std::list<PanelScreen*>user_screens;
	nanogui::Vector2i old_size;
	nanogui::Theme *theme;
	EditorSettings settings;
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
	SeriesWindow *w_buffers;
	using imagesDataType = std::vector<std::pair<GLTexture, GLTexture::handleType>>;
	imagesDataType mImagesData;
	bool needs_update;
	unsigned int sample_buffer_size;
};

class StructureFactoryButton : public SelectableButton {
public:
	StructureFactoryButton(EditorGUI *screen,
						const std::string type_str, Palette *pal,
						nanogui::Widget *parent,
						int object_type,
						const std::string &name, const std::string &addr_str)
	: SelectableButton(type_str, pal, parent, name),
	gui(screen), kind(object_type), tag_name(name), address_str(addr_str)
	{}
	nanogui::Widget *create(nanogui::Widget *container) const override;
private:
	EditorGUI *gui;
	int kind;
	std::string tag_name;
	std::string address_str;
};

class ObjectFactoryButton : public SelectableButton {
public:
	ObjectFactoryButton(EditorGUI *screen,
						const std::string type_str, Palette *pal,
						nanogui::Widget *parent,
						int object_type,
						const std::string &name, const std::string &addr_str)
	: SelectableButton(type_str, pal, parent, name),
	gui(screen), kind(object_type), tag_name(name), address_str(addr_str)
	{}
	nanogui::Widget *create(nanogui::Widget *container) const override;
	int getAddress();
	const std::string &tagName() { return tag_name; }
private:
	EditorGUI *gui;
	int kind;
	std::string tag_name;
	std::string address_str;
};


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

	void save(const std::string &path);

	EditorGUI *gui() { return screen; }

private:
	static Editor *_instance;
	bool mEditMode;
	bool mCreateMode;
	EditorGUI *screen;
	nanogui::DragHandle *drag_handle;
};

#define EDITOR Editor::instance()

Editor *Editor::_instance = 0;

class EditorWidget : public Selectable {

public:

	EditorWidget(nanogui::Widget *w) : Selectable(0), dh(0), handles(9), handle_coordinates(9,2) {
		assert(w != 0);
		Palette *p = dynamic_cast<Palette*>(w);
		if (!p) {
			//assert(EDITOR->gui()->getUserWindow()->getWindow() == w);
			p = EDITOR->gui()->getUserWindow();
		}
		this->palette = p;
	}

	nanogui::Widget *getWidget() { return widget; }
	virtual void loadProperties(PropertyFormHelper *pfh);

	virtual bool editorMouseButtonEvent(nanogui::Widget *widget, const nanogui::Vector2i &p, int button, bool down, int modifiers) {

		using namespace nanogui;

		if (EDITOR->isEditMode()) {
			if (down) {
				if (!mSelected) palette->clearSelections(); 
				if (mSelected && modifiers & GLFW_MOD_SHIFT)
					deselect();
				else
					select();
				
			}
			return false;
		}
		else {
			if (EDITOR->getDragHandle()) EDITOR->getDragHandle()->setVisible(false);
		}
		return true; // caller should continue to call the default handler for the object
	}

	virtual bool editorMouseMotionEvent(nanogui::Widget *widget, const nanogui::Vector2i &p, const nanogui::Vector2i &rel, int button, int modifiers) {
		if ( !EDITOR->isEditMode() )
			return true; // caller should continue to call the default handler for the object

		Eigen::Vector2d pt(p.x(), p.y());

		Eigen::VectorXd distances = (handle_coordinates.rowwise() - pt.transpose()).rowwise().squaredNorm();

		double min = distances.row(0).x();

		int idx = 0;

		for (int i=1; i<9; ++i) {
			if (distances.row(i).x() < min) {
				min = distances.row(i).x();
				idx = i;
			}
		}

		if (min >= 24) {
			EDITOR->getDragHandle()->setVisible(false);
			updateHandles(widget);
			return false;
		}

		nanogui::DragHandle *drag_handle = EDITOR->getDragHandle();
		if (handles[idx].mode() != Handle::NONE) {
			drag_handle->setTarget(widget);

			drag_handle->setPosition(
					Vector2i(handles[idx].position().x() - drag_handle->size().x()/2,
							 handles[idx].position().y() - drag_handle->size().y()/2) ) ;

			if (drag_handle->propertyMonitor())
				drag_handle->propertyMonitor()->setMode( handles[idx].mode() );

			drag_handle->setVisible(true);
		}
		else {
			drag_handle->setVisible(false);
			updateHandles(widget);
		}

		return false;
	}

	virtual bool editorMouseEnterEvent(nanogui::Widget *widget, const Vector2i &p, bool enter) {
		if (enter) updateHandles(widget);

		return true;
	}

	void updateHandles(nanogui::Widget *w) {
		for (int i=0; i<9; ++i) {
			Handle h = Handle::create(all_handles[i], w->position(), w->size());
			handle_coordinates(i,0) = h.position().x();
			handle_coordinates(i,1) = h.position().y();
			handles[i] = h;
		}
	}


	void setName(const std::string new_name) { name = new_name; }
	const std::string &getName() const { return name; }

	void justSelected() override;
	void justDeselected() override;

	void setPatterns(const std::string patterns) { pattern_list = patterns; }
	const std::string &patterns() const { return pattern_list; }

protected:
	std::string name;
	nanogui::DragHandle *dh;
	std::vector<Handle> handles;
	MatrixXd handle_coordinates;
	std::string pattern_list;
};



class EditorButton : public nanogui::Button, public EditorWidget {

public:

	EditorButton(Widget *parent, const std::string &caption = "Untitled", bool toggle = false, unsigned int modbus_address = 0, int icon = 0)
	: Button(parent, caption, icon), EditorWidget(this), is_toggle(toggle), addr(modbus_address) {
	}

	void loadProperties(PropertyFormHelper* properties) override;

	virtual bool mouseButtonEvent(const nanogui::Vector2i &p, int button, bool down, int modifiers) override {

		using namespace nanogui;
		if (editorMouseButtonEvent(this, p, button, down, modifiers))
			if (!is_toggle || (is_toggle && down))
				return nanogui::Button::mouseButtonEvent(p, button, down, modifiers);

		return true;
	}

	virtual bool mouseMotionEvent(const nanogui::Vector2i &p, const nanogui::Vector2i &rel, int button, int modifiers) override {

		if (editorMouseMotionEvent(this, p, rel, button, modifiers))
			return Button::mouseMotionEvent(p, rel, button, modifiers);

		return true;
	}

	virtual bool mouseEnterEvent(const Vector2i &p, bool enter) override {

		if (editorMouseEnterEvent(this, p, enter))
			return Button::mouseEnterEvent(p, enter);

		return true;
	}

/*	bool keyboardEvent(int key, int scancode, int action, int modifiers) override {
		if (focused() && isSelected() && action == GLFW_PRESS && (key == GLFW_KEY_BACKSPACE || key == GLFW_KEY_DELETE) ) {
			UserWindow *uw = EDITOR->gui()->getUserWindow();
			if (uw && uw->hasSelections()) {
				incRef();
				uw->deleteSelections();
				decRef();
				return false;
			}
		}
		return true;			
	}
*/
	int address() const {
		return addr;
	}

	virtual void draw(NVGcontext *ctx) override {
		nanogui::Button::draw(ctx);
		if (mSelected) {
			nvgStrokeWidth(ctx, 4.0f);
			nvgBeginPath(ctx);
			nvgRect(ctx, mPos.x(), mPos.y(), mSize.x(), mSize.y());
			nvgStrokeColor(ctx, nvgRGBA(80, 220, 0, 255));
			nvgStroke(ctx);
		}
	}


	bool is_toggle;
	unsigned int addr;
};

class EditorTextBox : public nanogui::TextBox, public EditorWidget {

public:

	EditorTextBox(Widget *parent, int modbus_address = 0, int icon = 0)
	: TextBox(parent), EditorWidget(this), dh(0), handles(9), handle_coordinates(9,2), addr(modbus_address) {
	}

	virtual bool mouseButtonEvent(const nanogui::Vector2i &p, int button, bool down, int modifiers) override {

		using namespace nanogui;

		if (editorMouseButtonEvent(this, p, button, down, modifiers))
			return nanogui::TextBox::mouseButtonEvent(p, button, down, modifiers);

		return true;
	}

	virtual bool mouseMotionEvent(const nanogui::Vector2i &p, const nanogui::Vector2i &rel, int button, int modifiers) override {

		if (editorMouseMotionEvent(this, p, rel, button, modifiers))
			return TextBox::mouseMotionEvent(p, rel, button, modifiers);

		return true;
	}

	virtual bool mouseEnterEvent(const Vector2i &p, bool enter) override {

		if (editorMouseEnterEvent(this, p, enter))
			return TextBox::mouseEnterEvent(p, enter);

		return true;
	}

	int address() const {
		return addr;
	}


	virtual void draw(NVGcontext *ctx) override {
		nanogui::TextBox::draw(ctx);
		if (mSelected) {
			nvgStrokeWidth(ctx, 4.0f);
			nvgBeginPath(ctx);
			nvgRect(ctx, mPos.x(), mPos.y(), mSize.x(), mSize.y());
			nvgStrokeColor(ctx, nvgRGBA(80, 220, 0, 255));
			nvgStroke(ctx);
		}
	}

	nanogui::DragHandle *dh;
	std::vector<Handle> handles;
	MatrixXd handle_coordinates;
	int addr;
};


class EditorLabel : public nanogui::Label, public EditorWidget {

public:

	EditorLabel(Widget *parent, const std::string caption, const std::string &font = "sans", int fontSize = -1, int modbus_address = 0, int icon = 0)
	: Label(parent, caption), EditorWidget(this), dh(0), handles(9), handle_coordinates(9,2), addr(modbus_address) {
	}

	virtual bool mouseButtonEvent(const nanogui::Vector2i &p, int button, bool down, int modifiers) override {

		using namespace nanogui;

		if (editorMouseButtonEvent(this, p, button, down, modifiers))
			return nanogui::Label::mouseButtonEvent(p, button, down, modifiers);

		return true;
	}

	virtual bool mouseMotionEvent(const nanogui::Vector2i &p, const nanogui::Vector2i &rel, int button, int modifiers) override {

		if (editorMouseMotionEvent(this, p, rel, button, modifiers))
			return Label::mouseMotionEvent(p, rel, button, modifiers);

		return true;
	}

	virtual bool mouseEnterEvent(const Vector2i &p, bool enter) override {

		if (editorMouseEnterEvent(this, p, enter))
			return Label::mouseEnterEvent(p, enter);

		return true;
	}

	int address() const {
		return addr;
	}


	virtual void draw(NVGcontext *ctx) override {
		nanogui::Label::draw(ctx);
		if (mSelected) {
			nvgStrokeWidth(ctx, 4.0f);
			nvgBeginPath(ctx);
			nvgRect(ctx, mPos.x(), mPos.y(), mSize.x(), mSize.y());
			nvgStrokeColor(ctx, nvgRGBA(80, 220, 0, 255));
			nvgStroke(ctx);
		}
	}

	nanogui::DragHandle *dh;
	std::vector<Handle> handles;
	MatrixXd handle_coordinates;
	int addr;
};


class EditorImageView : public nanogui::ImageView, public EditorWidget {

public:

	EditorImageView(Widget *parent, GLuint image_id, int modbus_address = 0, int icon = 0)
	: ImageView(parent, image_id), EditorWidget(this), dh(0), handles(9), handle_coordinates(9,2), addr(modbus_address) {
	}

	virtual bool mouseButtonEvent(const nanogui::Vector2i &p, int button, bool down, int modifiers) override {

		using namespace nanogui;

		if (editorMouseButtonEvent(this, p, button, down, modifiers))
			return nanogui::ImageView::mouseButtonEvent(p, button, down, modifiers);

		return true;
	}

	virtual bool mouseMotionEvent(const nanogui::Vector2i &p, const nanogui::Vector2i &rel, int button, int modifiers) override {

		if (editorMouseMotionEvent(this, p, rel, button, modifiers))
			return ImageView::mouseMotionEvent(p, rel, button, modifiers);

		return true;
	}

	virtual bool mouseEnterEvent(const Vector2i &p, bool enter) override {

		if (editorMouseEnterEvent(this, p, enter))
			return ImageView::mouseEnterEvent(p, enter);

		return true;
	}

	int address() const {
		return addr;
	}

	virtual void draw(NVGcontext *ctx) override {
		nanogui::ImageView::draw(ctx);
		if (mSelected) {
			nvgStrokeWidth(ctx, 4.0f);
			nvgBeginPath(ctx);
			nvgRect(ctx, mPos.x(), mPos.y(), mSize.x(), mSize.y());
			nvgStrokeColor(ctx, nvgRGBA(80, 220, 0, 255));
			nvgStroke(ctx);
		}
	}

	void setImageName(const std::string new_name) { image_name = new_name; }
	const std::string &imageName() const { return image_name; }

	nanogui::DragHandle *dh;
	std::vector<Handle> handles;
	MatrixXd handle_coordinates;
	int addr;
	std::string image_name;
};


class EditorLinePlot : public nanogui::LinePlot, public EditorWidget {

public:

	EditorLinePlot(Widget *parent, int modbus_address = 0, int icon = 0)
	: LinePlot(parent, "Test"), EditorWidget(this), handle_coordinates(9,2), addr(modbus_address),
		start_trigger_value(0), stop_trigger_value(0)
 	{
	}

	virtual bool mouseButtonEvent(const nanogui::Vector2i &p, int button, bool down, int modifiers) override {

		using namespace nanogui;

		if (editorMouseButtonEvent(this, p, button, down, modifiers))
			return nanogui::LinePlot::mouseButtonEvent(p, button, down, modifiers);

		return true;
	}

	virtual bool mouseMotionEvent(const nanogui::Vector2i &p, const nanogui::Vector2i &rel, int button, int modifiers) override {

		if (editorMouseMotionEvent(this, p, rel, button, modifiers))
			return nanogui::LinePlot::mouseMotionEvent(p, rel, button, modifiers);

		return true;
	}

	virtual bool mouseEnterEvent(const Vector2i &p, bool enter) override {

		if (editorMouseEnterEvent(this, p, enter))
			return nanogui::LinePlot::mouseEnterEvent(p, enter);

		return true;
	}

	virtual void loadProperties(PropertyFormHelper *pfh) override;

	int address() const {
		return addr;
	}

	virtual void draw(NVGcontext *ctx) override {
		nanogui::LinePlot::draw(ctx);
		if (mSelected) {
			nvgStrokeWidth(ctx, 4.0f);
			nvgBeginPath(ctx);
			nvgRect(ctx, mPos.x(), mPos.y(), mSize.x(), mSize.y());
			nvgStrokeColor(ctx, nvgRGBA(80, 220, 0, 255));
			nvgStroke(ctx);
		}
	}

	void setTriggerName(UserWindow *user_window, SampleTrigger::Event evt, const std::string name);
	void setTriggerValue(UserWindow *user_window, SampleTrigger::Event evt, int val);


	/*void setMonitors(const std::string mon) {
		monitored_objects = mon;
	}
	const std::string monitors() const { return monitored_objects; }
	*/
private:
	MatrixXd handle_coordinates;
	int addr;
	std::string name;
	std::string monitored_objects;
	std::string start_trigger_name;
	int start_trigger_value;
	std::string stop_trigger_name;
	int stop_trigger_value;
};

EditorGUI::EditorGUI() : theme(0), state(GUIWELCOME), editor(0), w_toolbar(0), w_properties(0), w_theme(0), w_user(0), w_patterns(0),
	w_structures(0), w_connections(0), w_startup(0), needs_update(false),
	sample_buffer_size(500)
{
	old_size = mSize;
	std::vector<std::pair<int, std::string>> icons = nanogui::loadImageDirectory(mNVGContext, "images");
	// Load all of the images by creating a GLTexture object and saving the pixel data.
	std::string resourcesFolderPath("./");
	for (auto& icon : icons) {
		GLTexture texture(icon.second);
		auto data = texture.load(resourcesFolderPath + icon.second + ".png");
		cout << "loaded image " << icon.second << " with id " << texture.texture() << "\n";
		mImagesData.emplace_back(std::move(texture), std::move(data));
	}
	assert(mImagesData.size() > 0);
}

class Toolbar : public nanogui::Window {
public:
	Toolbar(EditorGUI *screen, nanogui::Theme *);
	nanogui::Window *getWindow() { return this; }
	bool mouseDragEvent(const Vector2i &p, const Vector2i &rel, int button, int modifiers) override {
		bool res = nanogui::Window::mouseDragEvent(p, rel, button, modifiers);
		if (gui) gui->updateSettings(this);
		return res;
	}
private:
	EditorGUI *gui;
};

class StartupWindow : public Skeleton {
public:
	StartupWindow(EditorGUI *screen, nanogui::Theme *theme);
	void setVisible(bool which) { window->setVisible(which); }

private:
	std::string message;
	EditorGUI *gui;
};

class PropertyFormHelper : public nanogui::FormHelper {
public:
	PropertyFormHelper(nanogui::Screen *screen) : nanogui::FormHelper(screen), mContent(0) { }
	void clear() {
		while (window()->childCount())
			window()->removeChild(0);
	}

	nanogui::Window *addWindow(const Vector2i &pos,
							  const std::string &title = "Untitled") override {
		assert(mScreen);
		if (mWindow) mWindow->decRef();
		mWindow = new nanogui::Window(mScreen, title);
		mLayout = new nanogui::AdvancedGridLayout({10, 0, 10, 0}, {});
		nanogui::VScrollPanel *palette_scroller = new nanogui::VScrollPanel(mWindow);
		palette_scroller->setFixedSize(Vector2i(mWindow->width(), mWindow->height() - mWindow->theme()->mWindowHeaderHeight));
		palette_scroller->setPosition( Vector2i(5, mWindow->theme()->mWindowHeaderHeight+1));
		mContent = new nanogui::Widget(palette_scroller);
		mContent->setLayout(mLayout);
		mLayout->setMargin(10);
		mLayout->setColStretch(2, 1);
		mWindow->setPosition(pos);
		mWindow->setLayout( new nanogui::BoxLayout(nanogui::Orientation::Vertical) );
		mWindow->setVisible(true);
		return mWindow;
	}

	void setWindow(nanogui::Window *wind) override {
		assert(mScreen);
		mWindow = wind;
		mLayout = new nanogui::AdvancedGridLayout({10, 0, 10, 0}, {});
		mLayout->setMargin(10);
		mLayout->setColStretch(2, 1);
		nanogui::VScrollPanel *palette_scroller = new nanogui::VScrollPanel(mWindow);
		palette_scroller->setFixedSize(Vector2i(mWindow->width(), mWindow->height() - mWindow->theme()->mWindowHeaderHeight));
		palette_scroller->setPosition( Vector2i(5, mWindow->theme()->mWindowHeaderHeight+1));
		mContent = new nanogui::Widget(palette_scroller);
		mContent->setLayout(mLayout);
		mWindow->setLayout( new nanogui::BoxLayout(nanogui::Orientation::Vertical) );
		mWindow->setVisible(true);
	}

	nanogui::Widget *content() override { return mContent; }
private:
	nanogui::Widget *mContent;
};

class Proxy {
public:

};

class ItemProxy {
public:
	ItemProxy(PropertyFormHelper *pfh, nanogui::Widget *w) : helper(pfh), item(w) {
		getAll();
	}
	void getAll() {
		if (!item) return;
		x = item->position().x();
		y = item->position().y();
		//tbd
	}
	void setAll() {
		if (!item) return;
		item->setPosition(Eigen::Vector2i(x,y));
		//tbd
	}
	void link(nanogui::Widget *w) {
		item = w;
		getAll();
		helper->clear();
		helper->addVariable("x pos", x);
		helper->addVariable("y pos", y);
	}
	int x;
	int y;
private:
	PropertyFormHelper *helper;
	nanogui::Widget *item;
};

class PropertyWindow : public nanogui::Object {
public:
	PropertyWindow(nanogui::Screen *screen, nanogui::Theme *theme);
	void setVisible(bool which) { window->setVisible(which); }
	nanogui::Window *getWindow()  { return window; }
	PropertyFormHelper *getFormHelper() { return properties; }
	void update();
	nanogui::Screen *getScreen() { return screen; }
	void show(nanogui::Widget &w);

protected:
	nanogui::Screen *screen;
	nanogui::Window *window;
	PropertyFormHelper *properties;
	//ItemProxy *item_proxy;
};

/* an ObjectWindow is a palette of elements from a clockwork connection.

 The device can collect clockwork object from a tag file or from direct
 connection (TBD).
 */
class ObjectWindow : public Skeleton, public Palette {
public:
	ObjectWindow(EditorGUI *screen, nanogui::Theme *theme, const char *tag_fname = 0);
	void setVisible(bool which) { window->setVisible(which); }
	void update();
	nanogui::Screen *getScreen() { return gui; }
	void show(nanogui::Widget &w);
	void loadTagFile(const std::string tagfn);
	nanogui::Window *createPanelPage(const char *name,
									 const char *filename = 0,
									 nanogui::Widget *palette_content = 0);
	bool importModbusInterface(std::istream &init,
							   nanogui::Widget *palette_content,
							   nanogui::Widget *container);
	nanogui::Widget *getItems() { return items; }
protected:
	EditorGUI *gui;
	std::string tag_file_name;
	nanogui::Widget *items;
	nanogui::TextBox *search_box;
};

class ThemeWindow : public nanogui::Object {
public:
	ThemeWindow(nanogui::Screen *screen, nanogui::Theme *theme);
	void setVisible(bool which) { window->setVisible(which); }
	nanogui::Window *getWindow()  { return window; }
	void loadTheme(nanogui::Theme*);
private:
	nanogui::Window *window;
	nanogui::FormHelper *properties;
};

class StructuresWindow : public Skeleton, public Palette {
public:
	StructuresWindow(EditorGUI *screen, nanogui::Theme *theme);
	void setVisible(bool which) { window->setVisible(which); }
private:
	EditorGUI *gui;
};

class PatternsWindow : public Skeleton, public Palette {
public:
	PatternsWindow(EditorGUI *screen, nanogui::Theme *theme);
	void setVisible(bool which) { window->setVisible(which); }
private:
	EditorGUI *gui;
};

class ScreensWindow : public Skeleton, public Palette {
public:
	ScreensWindow(EditorGUI *screen, nanogui::Theme *theme);
	void setVisible(bool which) { window->setVisible(which); }
	void update();
private:
	EditorGUI *gui;
	nanogui::VScrollPanel *palette_scroller;
	nanogui::Widget *palette_content;
};

class ViewsWindow : public nanogui::Object {
public:
	ViewsWindow(EditorGUI *screen, nanogui::Theme *theme);
	void setVisible(bool which) { window->setVisible(which); }
	nanogui::Window *getWindow()  { return window; }
	void add(const std::string name, nanogui::Widget *);
private:
	EditorGUI *gui;
	nanogui::Window *window;
	nanogui::FormHelper *properties;
};

class SeriesWindow : public nanogui::Object {
public:
	SeriesWindow(EditorGUI *screen, nanogui::Theme *theme);
	void setVisible(bool which) { window->setVisible(which); }
	nanogui::Window *getWindow()  { return window; }
	void add(const std::string name, nanogui::TimeSeries *series);
	void update();
private:
	EditorGUI *gui;
	nanogui::Window *window;
	nanogui::FormHelper *properties;
};


Toolbar::Toolbar(EditorGUI *screen, nanogui::Theme *theme) : nanogui::Window(screen), gui(screen) {
	using namespace nanogui;
	Window *toolbar = this;
	toolbar->setTheme(theme);
	ToolButton *tb = new ToolButton(toolbar, ENTYPO_ICON_PENCIL);
	tb->setFlags(Button::ToggleButton);
	tb->setFixedSize(Vector2i(32,32));
	tb->setPosition(Vector2i(32, 64));
	tb->setChangeCallback([this](bool state) {
		Editor *editor = EDITOR;
		if (state)
			editor->gui()->setState(EditorGUI::GUIEDITMODE);
		else
			editor->gui()->setState(EditorGUI::GUIWORKING);
		if (editor) {
			editor->setEditMode(state);
		}
	});
	tb = new ToolButton(toolbar, ENTYPO_ICON_NEW);
	tb->setFlags(Button::ToggleButton);
	tb->setTooltip("New");
	tb->setFixedSize(Vector2i(32,32));
	tb = new ToolButton(toolbar, ENTYPO_ICON_OPEN_BOOK);
	tb->setFlags(Button::ToggleButton);
	tb->setTooltip("Open");
	tb->setFixedSize(Vector2i(32,32));
	tb->setCallback([&] {
		std::string tags(file_dialog(
		  { {"csv", "Clockwork TAG file"}, {"txt", "Text file"} }, false));
		gui->getObjectWindow()->loadTagFile(tags);
	});

	tb = new ToolButton(toolbar, ENTYPO_ICON_SAVE);
	//tb->setFlags(Button::ToggleButton);
	tb->setTooltip("Save");
	tb->setChangeCallback([this](bool state) {
		Editor *editor = EDITOR;
		if (editor) {
			std::string file_path(file_dialog(
				{ {"humid", "Humid layout file"},
				  {"txt", "Text file"} }, true));
			if (file_path.length()) editor->save(file_path);
		}
	});
	tb->setFixedSize(Vector2i(32,32));
	tb = new ToolButton(toolbar, ENTYPO_ICON_INSTALL);
	tb->setTooltip("Refresh");
	tb->setFixedSize(Vector2i(32,32));
	tb->setChangeCallback([](bool state) { });

	ToolButton *settings_button = new ToolButton(toolbar, ENTYPO_ICON_COG);
	settings_button->setFixedSize(Vector2i(32,32));
	settings_button->setTooltip("Theme properties");
	settings_button->setChangeCallback([this](bool state) { this->gui->getThemeWindow()->setVisible(state); });

	tb = new ToolButton(toolbar, ENTYPO_ICON_LAYOUT);
	tb->setTooltip("Create");
	tb->setFixedSize(Vector2i(32,32));
	tb->setChangeCallback([](bool state) { });

	tb = new ToolButton(toolbar, ENTYPO_ICON_EYE);
	tb->setTooltip("Views");
	tb->setFixedSize(Vector2i(32,32));
	tb->setChangeCallback([this](bool state) {
		this->gui->getViewsWindow()->setVisible(state);
		//this->gui->sendIODMessage("MODBUS REFRESH");
 	});

	BoxLayout *bl = new BoxLayout(Orientation::Horizontal);
	toolbar->setLayout(bl);
	toolbar->setTitle("Toolbar");

	toolbar->setVisible(false);
}


Editor::Editor(EditorGUI *gui) :mEditMode(false), mCreateMode(false), screen(gui)  {
	_instance = this;
	drag_handle = new nanogui::DragHandle(screen, new PositionMonitor);
}

Editor *Editor::instance() {
	return _instance;
}

void Editor::setEditMode(bool which) {
	mEditMode = which;
	if (!mEditMode) {
		if (drag_handle) drag_handle->setVisible(false);
		screen->getUserWindow()->clearSelections();
	}
}

bool Editor::isEditMode() {
	return mEditMode;
}

bool Editor::isCreateMode() {
	return mCreateMode;
}

void Editor::setCreateMode(bool which) {
	mCreateMode = which;
}

void Editor::refresh(bool ) {
	if (screen) {
		if (screen->getUserWindow() ) {
			nanogui::Window *mainWindow = screen->getUserWindow()->getWindow();
			nanogui::Theme *theme = mainWindow->theme();
			theme->incRef();
			mainWindow->setTheme(new nanogui::Theme(screen->nvgContext()));
			mainWindow->setTheme(theme);
			theme->decRef();
		}

		screen->performLayout();

	}
}

nanogui::DragHandle *Editor::getDragHandle() { return drag_handle; }

void Editor::save(const std::string &path) {
	using namespace nanogui;
	UserWindow *uw = screen->getUserWindow();
	if (uw) uw->save(path);
}

StartupWindow::StartupWindow(EditorGUI *screen, nanogui::Theme *theme) : Skeleton(screen), gui(screen) {
	using namespace nanogui;
	window->setTheme(theme);
	Label *itemText = new Label(window, "", "sans-bold");
	itemText->setPosition(Vector2i(40, 40));
	itemText->setSize(Vector2i(260, 20));
	itemText->setFixedSize(Vector2i(260, 20));
	itemText->setCaption("Select a project to open");
	itemText->setVisible(true);

	Button *newProjectButton = new Button(window, "New");
	newProjectButton->setPosition(Vector2i(80, 80));
	newProjectButton->setFixedSize(Vector2i(120, 40));
	newProjectButton->setSize(Vector2i(120, 40));
	newProjectButton->setCallback([this, newProjectButton] {
		std::cout << "New project\n";
		newProjectButton->parent()->setVisible(false);
		this->gui->setState(EditorGUI::GUICREATEPROJECT);
	});
	newProjectButton->setEnabled(true);

	window->setFixedSize(Vector2i(400, 400));
	window->setSize(Vector2i(400, 400));
	window->setVisible(false);
}

class UserWindowWin : public SkeletonWindow {
public:
	UserWindowWin(EditorGUI *s, const std::string caption) : SkeletonWindow(s, caption), gui(s) {
	}

	bool keyboardEvent(int key, int /* scancode */, int action, int modifiers);
	void update();

private:
	EditorGUI *gui;
};

bool UserWindowWin::keyboardEvent(int key, int scancode , int action, int modifiers) {
	if (action == GLFW_PRESS || action == GLFW_REPEAT) {
		if (EDITOR && EDITOR->isEditMode()) {
			if (action == GLFW_PRESS && (key == GLFW_KEY_BACKSPACE || key == GLFW_KEY_DELETE) ) {
				UserWindow *uw = gui->getUserWindow();
				if (uw && uw->hasSelections()) {
					uw->deleteSelections();
				}
			}
		}		
		else {
			for (auto item : children() ) {
				nanogui::LinePlot *lp = dynamic_cast<nanogui::LinePlot*>(item);
				if (lp) lp->handleKey(key, scancode, action, modifiers);
			}
		}
	}
	return true;
}

/*
UserWindow::UserWindow(EditorGUI *screen, nanogui::Theme *theme) : Skeleton(screen), gui(screen), current_layer(0), mDefaultSize(1024,768) {
	using namespace nanogui;
	gui = screen;
	window->setTheme(theme);
	window->setFixedSize(mDefaultSize);
	window->setSize(mDefaultSize);
	window->setVisible(false);
	window->setTitle("Untitled");
}
*/
UserWindow::UserWindow(EditorGUI *screen, nanogui::Theme *theme, UserWindowWin *uww)
: Skeleton(screen, uww), gui(screen), current_layer(0), mDefaultSize(1024,768) {
	using namespace nanogui;
	gui = screen;
	window->setTheme(theme);
	window->setFixedSize(mDefaultSize);
	window->setSize(mDefaultSize);
	window->setVisible(false);
	window->setTitle("Untitled");
	window->setPosition(nanogui::Vector2i(200,48));
	push(window);
	
	/* TBD
	TabWidget* tabWidget = window->add<TabWidget>();
	current_layer = tabWidget->createTab("Screen 1");
	layers.push_back(current_layer);
	 */
}

NVGcontext* UserWindow::getNVGContext() { return gui->nvgContext(); }

void UserWindow::deleteSelections() {
	if (EDITOR->getDragHandle()) EDITOR->getDragHandle()->setVisible(false);
	getWindow()->requestFocus();
	for (auto sel : getSelected()) {
		nanogui::Widget *w = dynamic_cast<nanogui::Widget*>(sel);
		if (w) getWindow()->removeChild(w);
	}
	clearSelections();
}

CircularBuffer *UserWindow::getValues(const std::string name) {
	auto iter = data.find(name);
	if (iter != data.end()) {
		const std::pair<std::string, CircularBuffer *> &node = *iter;
		return node.second;
	}
	else
		std::cout << "data series " << name << " not found\n";
	return 0;
}

CircularBuffer * UserWindow::addDataBuffer(const std::string name, size_t len) {
	std::cout << "adding data buffer for " << name << "\n";
	return data[name] = new CircularBuffer(len);
}

void UserWindow::save(const std::string &path) {
	using namespace nanogui;
	for (auto it = window->children().rbegin(); it != window->children().rend(); ++it) {
		Widget *child = *it;
		EditorButton *b = dynamic_cast<EditorButton*>(child);
		if (b) {
			std::cout << b->getName() << " BUTTON ("
				<< "pos_x: " << b->position().x() << ", pos_y: " << b->position().y()
				<< ", width: " << b->width() << ", height: " << b->height()
				<< ", caption: " << b->caption() << ");\n";
			continue;
		}
		EditorTextBox *t = dynamic_cast<EditorTextBox*>(child);
		if (t) {
			std::cout << t->getName() << " TEXT ("
			<< "pos_x: " << t->position().x() << ", pos_y: " << t->position().y()
			<< ", width: " << t->width() << ", height: " << t->height()
			<< ", value: " << t->value() << ");\n";
			continue;
		}
		EditorImageView *ip = dynamic_cast<EditorImageView*>(child);
		if (ip) {
			std::cout << ip->getName() << " IMAGE ("
			<< "pos_x: " << ip->position().x() << ", pos_y: " << ip->position().y()
			<< ", width: " << ip->width() << ", height: " << ip->height()
			<< ", caption: " << ip->imageName() << ");\n";
			continue;
		}
	}
}

void UserWindow::update() {
	UserWindowWin * w = dynamic_cast<UserWindowWin*>( getWindow() );
	if (w) w->update();
}

void UserWindowWin::update() {
	// if the window contains a line plot, that object should
	// be updated
	for (auto child : children()) {
		nanogui::LinePlot *lp = dynamic_cast<nanogui::LinePlot*>(child);
		lp->update();
	}
}

StructuresWindow::StructuresWindow(EditorGUI *screen, nanogui::Theme *theme) : Skeleton(screen), gui(screen) {
	using namespace nanogui;
	gui = screen;
	window->setTheme(theme);
	window->setFixedSize(Vector2i(180,240));
	window->setSize(Vector2i(180,240));
	window->setPosition(Vector2i(screen->width() - 200, 40));
	window->setLayout(new GridLayout(Orientation::Vertical,1));
	window->setTitle("Structures");
	window->setVisible(false);


	{
		VScrollPanel *palette_scroller = new VScrollPanel(window);
		int button_width = window->width() - 20;
		palette_scroller->setFixedSize(Vector2i(window->width(), window->height() - window->theme()->mWindowHeaderHeight));
		palette_scroller->setPosition( Vector2i(5, window->theme()->mWindowHeaderHeight+1));
		Widget *palette_content = new Widget(palette_scroller);
		palette_content->setLayout(new GridLayout(Orientation::Vertical,10));
		Widget *cell = new Widget(palette_content);
		cell->setFixedSize(Vector2i(button_width+4,35));

		SelectableButton *b = new StructureFactoryButton(gui, "BUTTON", this, cell, 0, "BUTTON", "");
		b->setEnabled(true);
		b->setFixedSize(Vector2i(button_width, 30));
		b->setPosition(Vector2i(2,2));

		cell = new Widget(palette_content);
		cell->setFixedSize(Vector2i(button_width+4,35));
		b = new StructureFactoryButton(gui, "IMAGE", this, cell, 0, "IMAGE", "");
		b->setEnabled(true);
		b->setFixedSize(Vector2i(button_width, 30));
		b->setPosition(Vector2i(2,2));

		cell = new Widget(palette_content);
		cell->setFixedSize(Vector2i(button_width+4,35));
		b = new StructureFactoryButton(gui, "RECT", this, cell, 0, "RECT", "");
		b->setEnabled(true);
		b->setFixedSize(Vector2i(button_width, 30));
		b->setPosition(Vector2i(2,2));

		cell = new Widget(palette_content);
		cell->setFixedSize(Vector2i(button_width+4,35));
		b = new StructureFactoryButton(gui, "LABEL", this, cell, 0, "LABEL", "");
		b->setEnabled(true);
		b->setFixedSize(Vector2i(button_width, 30));
		b->setPosition(Vector2i(2,2));

		cell = new Widget(palette_content);
		cell->setFixedSize(Vector2i(button_width+4,35));
		b = new StructureFactoryButton(gui, "TEXT", this, cell, 0, "TEXT", "");
		b->setEnabled(true);
		b->setFixedSize(Vector2i(button_width, 30));
		b->setPosition(Vector2i(2,2));

		cell = new Widget(palette_content);
		cell->setFixedSize(Vector2i(button_width+4,35));
		b = new StructureFactoryButton(gui, "PLOT", this, cell, 0, "PLOT", "");
		b->setEnabled(true);
		b->setFixedSize(Vector2i(button_width, 30));
		b->setPosition(Vector2i(2,2));
	}

}


PatternsWindow::PatternsWindow(EditorGUI *screen, nanogui::Theme *theme) : Skeleton(screen), gui(screen) {
	using namespace nanogui;
	gui = screen;
	window->setTheme(theme);
	window->setFixedSize(Vector2i(180,240));
	window->setSize(Vector2i(180,240));
	window->setPosition(Vector2i(screen->width() - 200, 420));
	window->setLayout(new GridLayout(Orientation::Horizontal,1));
	window->setTitle("Patterns");
	window->setVisible(false);

	ToolButton *tb = new ToolButton(window, ENTYPO_ICON_PENCIL);
	tb->setFlags(Button::ToggleButton);
	tb->setFixedSize(Vector2i(32,32));
	tb->setPosition(Vector2i(32, 64));



	{
		VScrollPanel *palette_scroller = new VScrollPanel(window);
		int button_width = window->width() - 20;
		palette_scroller->setFixedSize(Vector2i(window->width(), window->height() - window->theme()->mWindowHeaderHeight - 36));
		palette_scroller->setPosition( Vector2i(5, window->theme()->mWindowHeaderHeight+1));
		Widget *palette_content = new Widget(palette_scroller);
		palette_content->setLayout(new GridLayout(Orientation::Vertical,10));
		Widget *cell = new Widget(palette_content);
		cell->setFixedSize(Vector2i(button_width+4,35));
		SelectableButton *b = new SelectableButton("PATTERN", this, cell, "Centered");
		b->setEnabled(true);
		b->setFixedSize(Vector2i(button_width, 30));
		b->setPosition(Vector2i(2,2));

		cell = new Widget(palette_content);
		cell->setFixedSize(Vector2i(button_width+4,35));
		b = new SelectableButton("PATTERN", this, cell, "LargeText");
		b->setEnabled(true);
		b->setFixedSize(Vector2i(button_width, 30));
		b->setPosition(Vector2i(2,2));

		cell = new Widget(palette_content);
		cell->setFixedSize(Vector2i(button_width+4,35));
		b = new SelectableButton("PATTERN", this, cell, "Red");
		b->setEnabled(true);
		b->setFixedSize(Vector2i(button_width, 30));
		b->setPosition(Vector2i(2,2));


		cell = new Widget(palette_content);
		cell->setFixedSize(Vector2i(button_width+4,35));
		b = new SelectableButton("PATTERN", this, cell, "Align Horizontal");
		b->setEnabled(true);
		b->setFixedSize(Vector2i(button_width, 30));
		b->setPosition(Vector2i(2,2));

		cell = new Widget(palette_content);
		cell->setFixedSize(Vector2i(button_width+4,35));
		b = new SelectableButton("PATTERN", this, cell, "Align Vertical");
		b->setEnabled(true);
		b->setFixedSize(Vector2i(button_width, 30));
		b->setPosition(Vector2i(2,2));
}
	
}


ScreensWindow::ScreensWindow(EditorGUI *screen, nanogui::Theme *theme) : Skeleton(screen), gui(screen) {
	using namespace nanogui;
	gui = screen;
	window->setTheme(theme);
	window->setFixedSize(Vector2i(180,240));
	window->setSize(Vector2i(180,240));
	window->setPosition(Vector2i(screen->width() - 200, 40));
	window->setLayout(new GridLayout(Orientation::Vertical,1));
	window->setTitle("Screens");
	window->setVisible(false);
	{
		palette_scroller = new VScrollPanel(window);
		int button_width = window->width() - 20;
		palette_scroller->setFixedSize(Vector2i(window->width(), window->height() - window->theme()->mWindowHeaderHeight));
		palette_scroller->setPosition( Vector2i(5, window->theme()->mWindowHeaderHeight+1));
		palette_content = new Widget(palette_scroller);
		palette_content->setLayout(new GridLayout(Orientation::Vertical,10));
		Widget *cell = new Widget(palette_content);
		cell->setFixedSize(Vector2i(button_width+4,35));
	}
}

void ScreensWindow::update() {
	getWindow()->requestFocus();
	int button_width = window->width() - 20;
	int n = palette_content->childCount();
	while (n--) palette_content->removeChild(0);
	for (auto screen : gui->getScreens() ) {
		nanogui::Widget *cell = new nanogui::Widget(palette_content);
		cell->setFixedSize(Vector2i(button_width+4,35));
		SelectableButton *b = new SelectableButton("BUTTON", this, cell, screen->getName());
		b->setEnabled(true);
		b->setFixedSize(Vector2i(button_width, 30));
		b->setPosition(Vector2i(2,2));
	}
	getWindow()->performLayout(gui->nvgContext());
}


ViewsWindow::ViewsWindow(EditorGUI *screen, nanogui::Theme *theme) : gui(screen) {
	using namespace nanogui;
	properties = new FormHelper(screen);
	window = properties->addWindow(Eigen::Vector2i(200, 50), "Views");
	window->setTheme(theme);
	add("Structures", gui->getStructuresWindow()->getWindow());
	add("Properties", gui->getPropertyWindow()->getWindow());
	add("Objects", gui->getObjectWindow()->getWindow());
	add("Patterns", gui->getPatternsWindow()->getWindow());
	add("Screens", gui->getScreensWindow()->getWindow());
	window->setVisible(false);
}

void ViewsWindow::add(const std::string name, nanogui::Widget *w) {
	//gui->getViewManager().set(w, true);
	assert(w);
	properties->addVariable<bool> (
		name,
		  [&,w](bool value) mutable{
			  w->setVisible(value); gui->getViewManager().set(w, value);
		  },
		  [&,w]()->bool{ return gui->getViewManager().get(w).visible; });
}


SeriesWindow::SeriesWindow(EditorGUI *screen, nanogui::Theme *theme) : gui(screen) {
	using namespace nanogui;
	properties = new FormHelper(screen);
	window = properties->addWindow(Eigen::Vector2i(200, 50), "Views");
	window->setTheme(theme);
	window->setVisible(false);
}
/*
void SeriesWindow::update() {
	for (auto item : gui->getUserWindow()->getData()) {
		add(item.first, item.second);
	}

}

void SeriesWindow::add(const std::string name, nanogui::TimeSeries *series) {
	properties->addGroup(name);
	properties->addVariable<int> (
			"Line thickness",
		   [&,series](int value) mutable{
			   series.setLineWidth(value);
		   },
		   [&,series]()->int{ return series.getLineWidth(); });
}
*/

PropertyWindow::PropertyWindow(nanogui::Screen *s, nanogui::Theme *theme) : screen(s) {
	using namespace nanogui;
	properties = new PropertyFormHelper(screen);
	properties->setFixedSize(nanogui::Vector2i(120,28));
	//item_proxy = new ItemProxy(properties, 0);
	window = properties->addWindow(Eigen::Vector2i(30, 10), "Property List");
	window->setTheme(theme);
	window->setPosition(nanogui::Vector2i(32,64));
	window->setFixedSize(nanogui::Vector2i(260,560));
#if 0
	window->setLayout(new GridLayout(Orientation::Vertical,1));
	{
		VScrollPanel *palette_scroller = new VScrollPanel(window);
		int button_width = window->width() - 20;
		palette_scroller->setFixedSize(Vector2i(window->width(), window->height() - window->theme()->mWindowHeaderHeight));
		palette_scroller->setPosition( Vector2i(5, window->theme()->mWindowHeaderHeight+1));
		Widget *palette_content = new Widget(palette_scroller);
		palette_content->setLayout(new GridLayout(Orientation::Vertical,4));
		Widget *cell = new Widget(palette_content);
		cell->setFixedSize(Vector2i(button_width+4,35));
	}
#endif
	window->setVisible(false);


}
/*
void PropertyWindow::show(nanogui::Widget &w) {
	EditorGUI *gui = dynamic_cast<EditorGUI*>(screen);
	EditorWidget *ew = dynamic_cast<EditorWidget*>(&w);
	if (ew) {
		ew->loadProperties(properties);
	}
	else {
		properties->addVariable<int> ("Height",
									   [&](int value) mutable{ w.setHeight(value); },
									   [&]()->int{ return w.height(); });
		properties->addVariable<int> ("Width",
										[&](int value) mutable{ w.setWidth(value); },
										[&]()->int{ return w.width(); });
	}
	window->setVisible(true);
	gui->performLayout();
}
*/

void PropertyWindow::update() {
	//item_proxy->getAll();
	//properties->refresh();
	std::cout << "PropertyWindow::update\n";
#if 1
	EditorGUI *gui = dynamic_cast<EditorGUI*>(screen);
	UserWindow *uw = 0;
	if (gui) {
		uw = gui->getUserWindow();
		if (!uw) return;
		nanogui::Window *pw =getWindow();
		if (!pw) return;

		properties->clear();
		properties->setWindow(pw); // reset the grid layout
		int n = pw->children().size();

		if (uw->getSelected().size()) {
			for (auto sel : uw->getSelected()) {
				nanogui::Widget *w = dynamic_cast<nanogui::Widget*>(sel);
				EditorWidget *ew = dynamic_cast<EditorWidget*>(sel);
				if (ew) {
					ew->loadProperties(properties);
				}
				else if (w && sel->isSelected()) {
					std::string label("Width");
					properties->addVariable<int>(label,
								   [w](int value) { w->setWidth(value); },
								   [w]()->int{ return w->width(); });
				}
				break;
			}
		}
		else {
			std::string label("Screen Name");
			properties->addVariable<std::string>(label,
								   [uw](std::string value) {
									   PanelScreen *ps = uw->getActivePanel();
									   if (ps) {
										   ps->setName(value);
										   uw->getWindow()->setTitle(value);
									   }
									   if (uw->app()->getScreensWindow()) uw->app()->getScreensWindow()->update();
								   },
								   [uw]()->std::string{
									   PanelScreen *ps = uw->getActivePanel();
									   if (ps) return ps->getName();
									   return "";
								   });
			label = "Window Width";
			properties->addVariable<int>(label,
								   [uw](int value) mutable {
									   nanogui::Window *w = uw->getWindow(); 
									   w->setWidth(value);
									   w->setFixedWidth(value);
									   },
								   [uw]()->int{ return uw->getWindow()->width(); });
			label = "Window Height";
			properties->addVariable<int>(label,
								   [uw](int value) mutable {
									   nanogui::Window *w = uw->getWindow();
									   w->setHeight(value);
									   w->setFixedHeight(value);
									   },
								   [uw]()->int{ return uw->getWindow()->height(); });
		}
		n = pw->children().size();
		gui->performLayout();
	}
#endif
}

ObjectWindow::ObjectWindow(EditorGUI *screen, nanogui::Theme *theme, const char *tfn)
		: Skeleton(screen), gui(screen)
{
	using namespace nanogui;
	gui = screen;
	if (tfn) tag_file_name = tfn;
	window->setTheme(theme);
	window->setFixedSize(Vector2i(250, 600));
	window->setPosition( Vector2i(screen->width() - 260,48));
	window->setTitle("Objects");
	GridLayout *layout = new GridLayout(Orientation::Vertical,1, Alignment::Fill, 0, 3);
	window->setLayout(layout);
	items = new Widget(window);
	items->setPosition(Vector2i(1, window->theme()->mWindowHeaderHeight+1));
	items->setLayout(new BoxLayout(Orientation::Vertical));

	search_box = new nanogui::TextBox(items);
	search_box->setFixedSize(Vector2i(200, 25));
	search_box->setPosition( nanogui::Vector2i(5, 5));
	search_box->setValue("");
	search_box->setEnabled(true);
	search_box->setEditable(true);
	search_box->setAlignment(TextBox::Alignment::Left);
	search_box->setCallback([this](const std::string &filter)->bool {
		return false;
	});
	if (tag_file_name.length()) loadTagFile(tag_file_name);
	window->performLayout(screen->nvgContext());
	window->setVisible(false);
}

void ObjectWindow::loadTagFile(const std::string tags) {
	using namespace nanogui;
	const int search_height = 28;
	if (tags.length() && items->childCount() <= 1) {
		tag_file_name = tags;
		VScrollPanel *palette_scroller = new VScrollPanel(items);
		palette_scroller->setPosition( Vector2i(5,5)); //Vector2i(5, window->theme()->mWindowHeaderHeight+1 + search_height));
		Widget *palette_content = new Widget(palette_scroller);
		palette_content->setFixedSize(Vector2i(235, 600 - window->theme()->mWindowHeaderHeight-5 - search_height));

		createPanelPage("Objects", tag_file_name.c_str(), palette_content);
		GridLayout *palette_layout = new GridLayout(Orientation::Horizontal,1);
		palette_layout->setSpacing(4);
		palette_layout->setMargin(4);
		palette_layout->setColAlignment(nanogui::Alignment::Fill);
		palette_content->setLayout(palette_layout);
		palette_scroller->setFixedSize(Vector2i(240, 600 - window->theme()->mWindowHeaderHeight - search_height));

		window->performLayout(gui->nvgContext());
	}
}

ThemeWindow::ThemeWindow(nanogui::Screen *screen, nanogui::Theme *theme) {
	using namespace nanogui;
	properties = new FormHelper(screen);
	window = properties->addWindow(Eigen::Vector2i(10, 10), "Theme Properties");
	window->setTheme(theme);
	loadTheme(theme);
	window->setVisible(false);
}

void ThemeWindow::loadTheme(nanogui::Theme *theme) {
	properties->addVariable("Standard Font Size", theme->mStandardFontSize);
	properties->addVariable("Button Font Size", theme->mButtonFontSize);
	properties->addVariable("TextBox Font Size", theme->mTextBoxFontSize);
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
}

void setupTheme(nanogui::Theme *theme) {
	using namespace nanogui;
	theme->mStandardFontSize                 = 20;
	theme->mButtonFontSize                   = 20;
	theme->mTextBoxFontSize                  = 20;
	theme->mWindowCornerRadius               = 2;
	theme->mWindowHeaderHeight               = 30;
	theme->mWindowDropShadowSize             = 10;
	theme->mButtonCornerRadius               = 2;
	theme->mTabBorderWidth                   = 0.75f;
	theme->mTabInnerMargin                   = 5;
	theme->mTabMinButtonWidth                = 20;
	theme->mTabMaxButtonWidth                = 160;
	theme->mTabControlWidth                  = 20;
	theme->mTabButtonHorizontalPadding       = 10;
	theme->mTabButtonVerticalPadding         = 2;

	theme->mDropShadow                       = Color(0, 128);
	theme->mTransparent                      = Color(0, 0);
	theme->mBorderDark                       = Color(29, 255);
	theme->mBorderLight                      = Color(92, 255);
	theme->mBorderMedium                     = Color(35, 255);
	theme->mTextColor                        = Color(0, 160);
	theme->mDisabledTextColor                = Color(100, 80);
	theme->mTextColorShadow                  = Color(100, 160);
	theme->mIconColor                        = theme->mTextColor;

	theme->mButtonGradientTopFocused         = Color(255, 255);
	theme->mButtonGradientBotFocused         = Color(240, 255);
	theme->mButtonGradientTopUnfocused       = Color(240, 255);
	theme->mButtonGradientBotUnfocused       = Color(235, 255);
	theme->mButtonGradientTopPushed          = Color(235, 255);
	theme->mButtonGradientBotPushed          = Color(222, 255);

	/* Window-related */
	theme->mWindowFillUnfocused              = Color(245, 230);
	theme->mWindowFillFocused                = Color(255, 230);
	theme->mWindowTitleUnfocused             = theme->mDisabledTextColor;
	theme->mWindowTitleFocused               = theme->mTextColor;

	theme->mWindowHeaderGradientTop          = theme->mButtonGradientTopUnfocused;
	theme->mWindowHeaderGradientBot          = theme->mButtonGradientBotUnfocused;
	theme->mWindowHeaderSepTop               = theme->mBorderLight;
	theme->mWindowHeaderSepBot               = theme->mBorderDark;

	theme->mWindowPopup                      = Color(255, 255);
	theme->mWindowPopupTransparent           = Color(255, 0);
}

void EditorGUI::setTheme(nanogui::Theme *new_theme)  {  ClockworkClient::setTheme(new_theme); theme = new_theme; }

void EditorGUI::createWindows() {
	using namespace nanogui;
	editor = new Editor(this);

	w_theme = new ThemeWindow(this, theme);
	w_properties = new PropertyWindow(this, theme);
	UserWindowWin *uww = new UserWindowWin(this, "Untitled");
	user_screens.push_back(uww);
	nanogui::Theme *uwTheme = new nanogui::Theme(nvgContext());
	setupTheme(uwTheme);
	w_user = new UserWindow(this, uwTheme, uww);
	w_toolbar = new Toolbar(this, theme);
	w_startup = new StartupWindow(this, theme);
	w_objects = new ObjectWindow(this, theme);

	w_structures = new StructuresWindow(this, theme);

	w_patterns = new PatternsWindow(this, theme);
	w_screens = new ScreensWindow(this, theme);
	w_views = new ViewsWindow(this, theme);

	ConfirmDialog *s = new ConfirmDialog(this, "Humid V0.1");
	s->setCallback([s,this]{
		s->setVisible(false);
		this->setState(EditorGUI::GUISELECTPROJECT);
	});
	window = s->getWindow();
	window->setTheme(theme);

	settings.load("MainWindow", this);
	settings.load("ThemeSettings", w_theme->getWindow());
	settings.load("Properties", w_properties->getWindow());
	settings.load("Structures", w_structures->getWindow());
	settings.load("Patterns", w_patterns->getWindow());
	settings.load("Objects", w_objects->getWindow());
	settings.load("UserWindow", w_user->getWindow());
	settings.load("ScreensWindow", w_screens->getWindow());

	settings.add("MainWindow", this);
	settings.add("ThemeSettings", w_theme->getWindow());
	settings.add("Properties", w_properties->getWindow());
	settings.add("Structures", w_structures->getWindow());
	settings.add("Patterns", w_patterns->getWindow());
	settings.add("Objects", w_objects->getWindow());
	settings.add("UserWindow", w_user->getWindow());
	settings.add("ScreensWindow", w_screens->getWindow());

	uww->setMoveListener([this](nanogui::Window *value) {
		this->updateSettings(value);
	});
	w_objects->getSkeletonWindow()->setMoveListener(
		[this](nanogui::Window *value) { this->updateSettings(value); }
	);
	w_patterns->getSkeletonWindow()->setMoveListener(
													[this](nanogui::Window *value) { this->updateSettings(value); }
													);
	w_structures->getSkeletonWindow()->setMoveListener(
													 [this](nanogui::Window *value) { this->updateSettings(value); }
													 );

	w_screens->update();
	performLayout(mNVGContext);
}

void EditorWidget::justSelected() {
	EDITOR->gui()->getUserWindow()->select(this);
	PropertyWindow *prop = EDITOR->gui()->getPropertyWindow();
	if (prop) {
		//prop->show(*getWidget());
		prop->update();
	}
	EDITOR->gui()->getUserWindow()->getWindow()->requestFocus();
}

void EditorWidget::justDeselected() {
	EDITOR->gui()->getUserWindow()->deselect(this);
	PropertyWindow *prop = EDITOR->gui()->getPropertyWindow();
	if (prop) {
		prop->update();
		EDITOR->gui()->needsUpdate();
	}
}



void EditorGUI::setState(EditorGUI::GuiState s) {
	bool state = false;
	switch(s) {
		case GUIWELCOME:
			break;
		case GUISELECTPROJECT:
			window = getStartupWindow()->getWindow();
			window->setVisible(true);
			break;
		case GUICREATEPROJECT:
			getStartupWindow()->setVisible(false);
			getUserWindow()->setVisible(true);
			getToolbar()->setVisible(true);
			break;
		case GUIEDITMODE:
			state = true;
			// fall through
		case GUIWORKING:
			if (getPropertyWindow()) {
				nanogui::Window *w = getPropertyWindow()->getWindow();
				ViewOptions vo(views.get(w));
				w->setVisible(state && vo.visible);
			}
			if (getPatternsWindow()) {
				nanogui::Window *w = getPatternsWindow()->getWindow();
				w->setVisible(state && views.get(w).visible);
			}
			if (getStructuresWindow()) {
				nanogui::Window *w = getStructuresWindow()->getWindow();
				w->setVisible(state && views.get(w).visible);
			}
			if (getObjectWindow()) {
				nanogui::Window *w = getObjectWindow()->getWindow();
				w->setVisible(state && views.get(w).visible);
			}
			if (getScreensWindow()) {
				nanogui::Window *w = getScreensWindow()->getWindow();
				w->setVisible(state && views.get(w).visible);
			}
			break;
	}
	state = s;
}

nanogui::Vector2i fixPlacement(nanogui::Widget *w, nanogui::Widget *container, nanogui::Vector2i &pos) {
	if (pos.x() < 0) pos = Vector2i(0, pos.y());
	if (pos.x() + w->width() > container->width()) pos = Vector2i(container->width() - w->width(), pos.y());
	if (pos.y() < w->theme()->mWindowHeaderHeight) pos = Vector2i(pos.x(), w->theme()->mWindowHeaderHeight+1);
	if (pos.y() + w->height() > container->height()) pos = Vector2i(pos.x(), container->height() - w->height());
	return pos;
}

void EditorGUI::createStructures(const nanogui::Vector2i &p, std::set<Selectable *> selections) {
	using namespace nanogui;

	Widget *window = w_user->getWindow();
	DragHandle *drag_handle = editor->getDragHandle();
	
	drag_handle->incRef();
	removeChild(drag_handle);
	PropertyMonitor *pm = drag_handle->propertyMonitor();
	drag_handle->setPropertyMonitor(0);

	unsigned int offset = 0;
	for(Selectable *sel : selections) {
		SelectableButton *item = dynamic_cast<SelectableButton*>(sel);
		if (!item) continue;
		std::cout << "creating instance of " << item->getClass() << "\n";
		nanogui::Widget *w = item->create(window);
		if (w) {
			Vector2i pos(p - window->position() - w->size()/2 + nanogui::Vector2i(0, offset));
			w->setPosition(fixPlacement(w, window, pos));
			offset += w->height() + 8;
		}
	}
	window->addChild(drag_handle);

	drag_handle->setPropertyMonitor(pm);
	drag_handle->decRef();
}

bool EditorGUI::mouseButtonEvent(const nanogui::Vector2i &p, int button, bool down, int modifiers) {

	using namespace nanogui;

	Widget *clicked = findWidget(p);

	nanogui::Window *window = w_user->getWindow();
	Widget *ww = dynamic_cast<Widget*>(window);

	if (button != GLFW_MOUSE_BUTTON_1 || !window->contains(p /*- window->position()*/))
		return Screen::mouseButtonEvent(p, button, down, modifiers);


	bool is_child = false;
	nanogui::Vector2i pos(p - window->position());
	for (auto elem : window->children()) {
		if (elem->contains(pos)) { is_child = true; break; }
	}

	nanogui::DragHandle *drag_handle = editor->getDragHandle();

	if (drag_handle && EDITOR->isEditMode()) {
		if (clicked == ww && !is_child) {
			if (getStructuresWindow()->hasSelections()) {
				if (down) {
					createStructures(p, getStructuresWindow()->getSelected());
					getStructuresWindow()->clearSelections();
				}
				return false;
			}
			else if (getObjectWindow()->hasSelections()) {
				if (down) {
					createStructures(p, getObjectWindow()->getSelected());
					getObjectWindow()->clearSelections();
				}
				return false;
			}
			else {
				if (down) {
					window->requestFocus();
					getUserWindow()->clearSelections();
					if (getPropertyWindow()) getPropertyWindow()->update();
				}
				return Screen::mouseButtonEvent(p, button, down, modifiers);
			}
		}
		else {
			if (down) {
				if (drag_handle) drag_handle->setVisible(false);
				requestFocus();
				// deselect items on the window
				if (w_user->hasSelections() && (modifiers & GLFW_MOD_CONTROL)) {
					auto selected = w_user->getSelected();
					auto tmp = selected;
					for (auto sel : tmp) {
						sel->deselect();
						nanogui::Widget *w = dynamic_cast<nanogui::Widget*>(sel);
					}
					if (getPropertyWindow())
						getPropertyWindow()->update();
				}
				return Screen::mouseButtonEvent(p, button, down, modifiers);
			}
		}
		return true;
	}
	else {
		//not edit mode
		//window->requestFocus();
		return Screen::mouseButtonEvent(p, button, down, modifiers);
	}
}

GLuint EditorGUI::getImageId(const char *name) {
	for(auto it = mImagesData.begin(); it != mImagesData.end(); ++it) {
		const GLTexture &tex = (*it).first;
		if (tex.textureName() == name) {
			cout << "found image " << tex.textureName() << "\n";
			return tex.texture();
		}
	}
	return 0;
}

bool EditorGUI::resizeEvent(const Vector2i &new_size) {
	if (old_size == new_size) {
		return false;
	}
	nanogui::Window * windows[] = {
		this->getStructuresWindow()->getWindow(),
		this->getPropertyWindow()->getWindow(),
		this->getPatternsWindow()->getWindow(),
		this->getThemeWindow()->getWindow(),
		this->getViewsWindow()->getWindow(),
		this->getObjectWindow()->getWindow(),
		this->getScreensWindow()->getWindow()
	};
	std::list<std::pair<nanogui::Window *, nanogui::Vector2i>> positions;
	for (unsigned int i = 0; i<5; ++i) {
		nanogui::Window *w = windows[i];
		positions.push_back( std::make_pair(w, nanogui::Vector2i(w->position().x(), w->position().y())) );
	}
	float x_scale = (float)new_size.x() / (float)old_size.x();
	float y_scale = (float)new_size.y() / (float)old_size.y();
	bool res = nanogui::Screen::resizeEvent(new_size);
	for (auto it = positions.begin(); it != positions.end(); ++it) {
		std::pair<nanogui::Window *, nanogui::Vector2i> item = *it;
		nanogui::Window *w = item.first;
		nanogui::Vector2i pos = item.second;

		// items closer to the rhs stay the same distance from the rhs after scaling
		// similarly for items close to the lhs
		//pos.x() = (int) ((float)item.second.x() * x_scale);
		//pos.y() = (int) ((float)item.second.y() * y_scale);

		int lhs = pos.x(), rhs = old_size.x() - pos.x() - w->width();
		if(rhs < lhs) pos.x() = new_size.x() - rhs - w->width();

		// similarly for vertical offset
		//int top = pos.y(), bot = old_size.y() - pos.y() - w->height();
		//if(bot < lhs) pos.x() = new_size.x() - rhs - w->width();
		pos.y() = (int) ((float)item.second.y() * y_scale);

		item.first->setPosition(pos);
	}
	//cout << "\n";
	old_size = mSize;
	return res;
}

void EditorGUI::handleClockworkMessage(unsigned long now, const std::string &op, std::list<Value> *message) {
	if (op == "UPDATE") {
		if (!this->getUserWindow()) return;

		int pos = 0;
		std::string name;
		long val = 0;
		double dval = 0.0;
		CircularBuffer *buf = 0;
		for (auto &v: *message) {
			if (pos == 2) {
				name = v.asString();
				buf = w_user->getValues(name);
				if (!buf) {
					buf = w_user->addDataBuffer(name, sample_buffer_size);
				}
			}
			else if (buf && pos == 4) {
				if (v.asInteger(val)) {
					buf->addSample(now, (int16_t)(val & 0xffff));
					//std::cout << "adding sample: " << name << " t: " << now << " " << (int16_t)(val & 0xffff) << " count: " << buf->length() << "\n";
				}
				else if (v.asFloat(dval)) {
					buf->addSample(now, dval);
				}
			}
			++pos;
		}
	}
	//else
	//	std::cout << op << "\n";
}

void EditorGUI::update() { 
	if (w_user) w_user->update();
	if (needs_update) {
		w_properties->getWindow()->performLayout(nvgContext());
		needs_update = false;
	}
}

bool EditorGUI::keyboardEvent(int key, int scancode , int action, int modifiers) {
	if (action == GLFW_PRESS && (key == GLFW_KEY_BACKSPACE || key == GLFW_KEY_DELETE) ) {
		if (w_user->hasSelections()) {
			if (w_user->getWindow()->focused()) {
				w_user->deleteSelections();
				return false;
			}
		}
	}
	return Screen::keyboardEvent(key, scancode, action, modifiers);
}


CircularBuffer *UserWindow::getDataBuffer(const std::string item) {
	std::map<std::string, CircularBuffer *>::iterator found = data.find(item);
	if (found == data.end())
		addDataBuffer(item, gui->sampleBufferSize());
	return data[item];
}

nanogui::Window *ObjectWindow::createPanelPage(
		   const char *name,
		   const char *filename,
		   nanogui::Widget *palette_content) {
	using namespace nanogui;

	if (filename && palette_content) {
		std::string modbus_settings(filename);
		std::ifstream init(modbus_settings);

		if (init.good()) {
			importModbusInterface(init, palette_content, window);
		}
	}

	return window;
}
// ref http://stackoverflow.com/questions/7302996/changing-the-delimiter-for-cin-c

struct comma_is_space : std::ctype<char> {
	comma_is_space() : std::ctype<char>(get_table()) {}

	static mask const* get_table() {
		static mask rc[table_size];
		rc[(int)','] = std::ctype_base::space;
		return &rc[0];
	}
};

nanogui::Widget *StructureFactoryButton::create(nanogui::Widget *window) const {
	using namespace nanogui;
	Widget *result = 0;
	int object_width = 60;
	int object_height = 40;
	if (this->getClass() == "BUTTON") {
		EditorButton *b = new EditorButton(window, caption(), 0);
		b->setBackgroundColor(Color(200, 30, 30, 255));
		result = b;
	}
	else if (getClass() == "LABEL") {
		EditorLabel *eb = new EditorLabel(window, "untitled");
		result = eb;
	}
	else if (getClass() == "TEXT") {
		EditorTextBox *eb = new EditorTextBox(window);
		eb->setEditable(true);
		result = eb;
	}
	else if (getClass() == "IMAGE") {
		GLuint img = gui->getImageId("images/blank");
		EditorImageView *iv = new EditorImageView(window, img);
		iv->setGridThreshold(20);
		iv->setPixelInfoThreshold(20);
		result = iv;
	}
	else if (getClass() == "PLOT") {
		EditorLinePlot *lp = new EditorLinePlot(window);
		lp->setBufferSize(gui->sampleBufferSize());
		object_width = 200;
		object_height = 120;
		result = lp;
	}
	if (result) {
		result->setFixedSize(Vector2i(object_width, object_height));
		result->setSize(Vector2i(object_width, object_height));
	}
	return result;
}


void EditorWidget::loadProperties(PropertyFormHelper* properties) {
	nanogui::Widget *w = dynamic_cast<nanogui::Widget*>(this);
	if (w) {
		properties->addVariable<int> ("Horizontal Pos",
									  [&,w](int value) mutable{
										  Eigen::Vector2i pos(value, w->position().y());
										  w->setPosition(pos);
									  },
									  [&,w]()->int{ return w->position().x(); });
		properties->addVariable<int> ("Vertical Pos",
									  [&,w](int value) mutable{
										  Eigen::Vector2i pos(w->position().x(), value);
										  w->setPosition(pos);
									  },
									  [&,w]()->int{ return w->position().y(); });
		properties->addVariable<int> ("Width",
									  [&,w](int value) mutable{ w->setWidth(value); },
									  [&,w]()->int{ return w->width(); });
		properties->addVariable<int> ("Height",
								  [&,w](int value) mutable{ w->setHeight(value); },
								  [&,w]()->int{ return w->height(); });
		properties->addVariable<std::string> ("Name",
											  [&](std::string value) mutable{ setName(value); },
											  [&]()->std::string{ return getName(); });
		properties->addVariable<int> ("FontSize",
									  [&,w](int value) mutable{ w->setFontSize(value); },
									  [&,w]()->int{ return w->fontSize(); });
		properties->addVariable<std::string> ("Patterns",
											  [&,w](std::string value) mutable{ setPatterns(value); },
											  [&,w]()->std::string{ return patterns(); }
											  );

	}
}

void EditorButton::loadProperties(PropertyFormHelper* properties) {
	EditorWidget::loadProperties(properties);
	nanogui::Button *btn = dynamic_cast<nanogui::Button*>(this);
	if (btn) {
		EditorGUI *gui = EDITOR->gui();
		properties->addVariable<nanogui::Color> (
				 "Background colour",
			 [&,btn](const nanogui::Color &value) mutable{ btn->setBackgroundColor(value); },
			 [&,btn]()->const nanogui::Color &{ return btn->backgroundColor(); });
		properties->addVariable<nanogui::Color> (
		"Text colour",
		[&,btn](const nanogui::Color &value) mutable{ btn->setTextColor(value); },
		[&,btn]()->const nanogui::Color &{ return btn->textColor(); } );
		properties->addVariable<std::string> (
												 "Caption",
												 [&](std::string value) mutable{ setCaption(value); },
												 [&]()->std::string{ return caption(); });
		properties->addVariable<std::string> (
												 "Remote object",
												 [&](std::string value) mutable{ setName(value); },
												 [&]()->std::string{ return getName(); });
		properties->addVariable<unsigned int> (
												 "Modbus address",
												 [&](unsigned int value) mutable{ addr = value; },
											   [&]()->unsigned int{ return addr; });
		properties->addVariable<unsigned int> (
											   "Behaviour",
											   [&](unsigned int value) mutable{ setFlags(value & 0x0f); },
											   [&]()->unsigned int{ return flags(); });
	}
}

void EditorLinePlot::loadProperties(PropertyFormHelper* properties) {
	EditorWidget::loadProperties(properties);
	EditorGUI *gui = EDITOR->gui();

	properties->addVariable<float> ("X scale",
									[&](float value) mutable{ x_scale = value; },
									[&]()->float { return x_scale; });
	properties->addVariable<float> ("X offset",
									[&](float value) mutable{ x_scroll = value; },
									[&]()->float { return x_scroll; });
	properties->addVariable<bool> ("Overlay plots",
									[&](bool value) mutable{ overlay_plots = value; },
									[&]()->bool { return overlay_plots; });
	properties->addVariable<std::string> ("Monitors",
											 [&, gui](std::string value) mutable{
												 setMonitors( gui->getUserWindow(), value); },
											 [&, gui]()->std::string{ return monitors(); });
	properties->addButton("Monitor Selected", [&,gui]() mutable{
		if (gui->getObjectWindow()->hasSelections()) {
			requestFocus();
			std::string items;
			for (auto sel : gui->getObjectWindow()->getSelected()) {
				if (items.length()) items += ",";
				ObjectFactoryButton *btn = dynamic_cast<ObjectFactoryButton*>(sel);
				if (btn) {
					items += btn->tagName();
				}
			}
			if (items.length()) {
				setMonitors( gui->getUserWindow(), items );
				gui->getObjectWindow()->clearSelections();
			}

			gui->getPropertyWindow()->update();
		}
	});
	for (auto series : data) {
		properties->addGroup(series->getName());
		properties->addVariable<int> (
			  "Line style",
			  [&,series](int value) mutable{
				  series->setLineStyle(static_cast<nanogui::TimeSeries::LineStyle>(value));
			  },
			  [&,series]()->int{ return (int)series->getLineStyle(); });
		properties->addVariable<float> (
									  "Line thickness",
									  [&,series](float value) mutable{
										  series->setLineWidth(value);
									  },
									  [&,series]()->float{ return series->getLineWidth(); });
	}
	properties->addVariable<std::string> ("Start trigger",
										  [&,gui](const std::string value) mutable{
											  setTriggerName(gui->getUserWindow(),
															 SampleTrigger::START, value);},
			[&]()->std::string { return start_trigger_name; });
	properties->addVariable<int> ("Start Value",
								  [&,gui](int value) mutable{
									  setTriggerValue(gui->getUserWindow(),
													  SampleTrigger::START, value); },
				[&]()->int { return start_trigger_value; });
	properties->addVariable<std::string> ("Stop trigger",
										  [&,gui](const std::string value) mutable{
											  setTriggerName(gui->getUserWindow(),
															 SampleTrigger::STOP, value);},
										  [&,gui]()->std::string { return start_trigger_name; });
	properties->addVariable<int> ("Stop Value",
								  [&](int value) mutable{
									  setTriggerValue(gui->getUserWindow(),
													  SampleTrigger::STOP, value); },
									[&]()->int { return stop_trigger_value; });
}


void EditorLinePlot::setTriggerName(UserWindow *user_window, SampleTrigger::Event evt, const std::string name) {

	if (!user_window) return;

	CircularBuffer *buf = user_window->getDataBuffer(name);
	SampleTrigger *t = buf->getTrigger(evt);
	if (!t) {
		t = new SampleTrigger(name, 0);
		if (evt == SampleTrigger::START) t->setTriggerValue(start_trigger_value);
		else if (evt == SampleTrigger::STOP) t->setTriggerValue(stop_trigger_value);
		buf->setTrigger(t, evt);
	}
	else
		t->setPropertyName(name);
}

void EditorLinePlot::setTriggerValue(UserWindow *user_window, SampleTrigger::Event evt, int val) {
	if (evt == SampleTrigger::START) start_trigger_value = val;
	else if (evt == SampleTrigger::STOP) stop_trigger_value = val;
}


nanogui::Widget *ObjectFactoryButton::create(nanogui::Widget *container) const {
	using namespace nanogui;
	Widget *result = 0;
	int object_width = 60;
	int object_height = 25;
	switch (kind) {

		case '0': {
			size_t p = tag_name.find(".cmd_");

			if (p != std::string::npos) {
				char *rest = 0;
				int address_int = (int)strtol(address_str.c_str()+2,&rest, 10);

				if (*rest) {
					std::cerr << "Unexpected data in address\n";
					address_int = 0;
				}

				EditorButton *b = new EditorButton(container, tag_name.substr(p+5), false,
												   address_int);
				b->setSize(Vector2i(object_width, object_height));
				b->setName(tag_name);

				b->setChangeCallback([b, this] (bool state) {
					gui->queueMessage(
									  gui->getIODSyncCommand(0, b->address(), 1));
					gui->queueMessage(
									  gui->getIODSyncCommand(0, b->address(), 0));

				});
				result = b;
			}
			else {
				EditorButton *b = new EditorButton(container);
				b->setSize(Vector2i(object_width, object_height));
				b->setFlags(Button::ToggleButton);
				b->setName(tag_name);
				b->setChangeCallback([this,b](bool state) {
					gui->queueMessage(gui->getIODSyncCommand(0, b->address(), (state)?1:0));
					if (state)
						b->setBackgroundColor(Color(255, 80, 0, 255));
					else
						b->setBackgroundColor(b->theme()->mButtonGradientBotUnfocused   );
				});
				result = b;
			}
		}

			break;

		case '1': {
			char *rest = 0;
			int address_int = (int)strtol(address_str.c_str()+2,&rest, 10);
			EditorButton *b = new EditorButton(container, tag_name, true, address_int);
			b->setSize(Vector2i(object_width, object_height));
			b->setFixedSize(Vector2i(object_width, object_height));
			b->setFlags(Button::ToggleButton);
			b->setName(tag_name);
			b->setChangeCallback([this,b](bool state) {
				gui->queueMessage(gui->getIODSyncCommand(0, b->address(), (state)?1:0));
				if (state)
					b->setBackgroundColor(Color(255, 80, 0, 255));
				else
					b->setBackgroundColor(b->theme()->mButtonGradientBotUnfocused   );
			});
			result = b;
		}

			break;

		case '3': {
			//EditorLabel *el = new EditorLabel(container, tag_name, std::string("sans-bold"));
			//el->setSize(Vector2i(object_width, object_height));
			//el->setFixedSize(Vector2i(object_width, object_height));
			//CheckBox *b = new CheckBox(container, tag_name);
			//img->setPolicy(ImageView::SizePolicy::Expand);
			//b->setBackgroundColor(Color(200, 200, 200, 255));
			//b->setFixedSize(Vector2i(25, 25));
			//result = new Label(container, tag_name, "sans-bold");

			EditorTextBox *textBox = new EditorTextBox(container);
			textBox->setName(tag_name);
			textBox->setValue("");
			textBox->setEnabled(true);
			textBox->setEditable(true);
			textBox->setSize(Vector2i(object_width, object_height));
			result = textBox;
		}

			break;

		case '4': {
			//new Label(container, tag_name, "sans-bold");
			auto textBox = new IntBox<int>(container);
			textBox->setEditable(true);
			textBox->setSize(Vector2i(object_width, object_height));
			textBox->setFixedSize(Vector2i(object_width, object_height));
			textBox->setValue(50);
			textBox->setDefaultValue("0");
			//textBox->setFontSize(16);
			textBox->setFormat("[1-9][0-9]*");
			textBox->setSpinnable(true);
			textBox->setMinValue(1);
			textBox->setValueIncrement(1);
			result = textBox;
		}

			break;

		default:
			;
	}
	return result;
}

bool ObjectWindow::importModbusInterface(std::istream &init,
										 nanogui::Widget *palette_content,
										 nanogui::Widget *container) {

	using namespace nanogui;

	if (!init.good()) return false;

	char buf[200];

	std::istringstream iss;

	iss.imbue(locale(iss.getloc(), new comma_is_space));

	unsigned int pallete_row = 0;

	while (init.getline(buf, 200, '\n')) {
		iss.str(buf);
		iss.clear();
		int protocol_id;
		std::string device_name, tag_name, data_type;
		int data_count;
		std::string retentive;
		std::string address_str;
		int array_start;
		int array_end;

		iss >> protocol_id >> device_name >> tag_name >> data_type
		>> data_count >> retentive >> address_str >> array_start
		>> array_end;
		char kind = address_str[1];
		{
			Widget *cell = new Widget(palette_content);
			cell->setFixedSize(Vector2i(210,35));

			SelectableButton *b = new ObjectFactoryButton(gui, "BUTTON", this, cell, kind, tag_name, address_str);
			b->setEnabled(true);
			b->setFixedSize(Vector2i(200, 30));
			//b->setPosition(Vector2i(100,++pallete_row * 32));
		}

		cout << kind << " " << tag_name << " "

		<< data_type << " " << data_count << " " << address_str
		<< "\n";
	}
	//palette_content->setFixedSize( nanogui::Vector2i( (pallete_row+1) * 32, palette_content->fixedWidth()));
	
	return true;
}

StartupWindow *EditorGUI::getStartupWindow() { return w_startup; }
PropertyWindow *EditorGUI::getPropertyWindow() { return w_properties; }
ObjectWindow *EditorGUI::getObjectWindow() { return w_objects; }
ThemeWindow *EditorGUI::getThemeWindow() { return w_theme; }
UserWindow *EditorGUI::getUserWindow() { return w_user; }
Toolbar *EditorGUI::getToolbar() { return w_toolbar; }
StructuresWindow *EditorGUI::getStructuresWindow() { return w_structures; }
PatternsWindow *EditorGUI::getPatternsWindow() { return w_patterns; }
ScreensWindow *EditorGUI::getScreensWindow() { return w_screens; }
ViewsWindow *EditorGUI::getViewsWindow() { return w_views; }

void loadSettingsFiles(std::list<std::string> &files) {

	/* load configuration from files named on the commandline */
	int opened_file = 0;
	std::list<std::string>::iterator f_iter = files.begin();
	while (f_iter != files.end())
	{
		const char *filename = (*f_iter).c_str();
		if (filename[0] != '-')
		{
			opened_file = 1;
			st_yyin = fopen(filename, "r");
			if (st_yyin)
			{
				std::cerr << "Processing file: " << filename << "\n";
				st_yylineno = 1;
				st_yycharno = 1;
				st_yyfilename = filename;
				st_yyparse();
				fclose(st_yyin);
			}
			else
			{
				std::stringstream ss;
				ss << "## - Error: failed to load config: " << filename;
				error_messages.push_back(ss.str());
				++num_errors;
			}
		}
		else if (strlen(filename) == 1) /* '-' means stdin */
		{
			opened_file = 1;
			std::cerr << "\nProcessing stdin\n";
			st_yyfilename = "stdin";
			st_yyin = stdin;
			st_yylineno = 1;
			st_yycharno = 1;
			st_yyparse();
		}
		f_iter++;
	}
}

int main(int argc, const char ** argv ) {
	char *pn = strdup(argv[0]);
	program_name = strdup(basename(pn));
	free(pn);

	zmq::context_t context;
	MessagingInterface::setContext(&context);

	int cw_port;
	std::string hostname;

	setup_signals();

	po::options_description desc("Allowed options");
	desc.add_options()
	("help", "produce help message")
	("debug",po::value<int>(&debug)->default_value(0), "set debug level")
	("host", po::value<std::string>(&hostname)->default_value("localhost"),"remote host (localhost)")
	("cwout",po::value<int>(&cw_port)->default_value(5555), "clockwork outgoing port (5555)")
	("cwin", "clockwork incoming port (deprecated)")
	;
	po::variables_map vm;
	po::store(po::parse_command_line(argc, argv, desc), vm);
	po::notify(vm);

	if (vm.count("help")) {
		std::cout << desc << "\n";
		return 1;
	}

	if (vm.count("cwout")) cw_out = vm["cwout"].as<int>();
	if (vm.count("host")) host = vm["host"].as<std::string>();
	if (vm.count("debug")) debug = vm["debug"].as<int>();
	if (DEBUG_BASIC) std::cout << "Debugging\n";

	std::string home(".");
	char *home_env = nullptr;
	if ( (home_env = getenv("HOME")) != nullptr )
		home = home_env;
	std::string fname(home);
	fname += "/.humidrc";
	settings_files.push_back(fname);
	loadSettingsFiles(settings_files);


	gettimeofday(&start, 0);

	try {
		nanogui::init();

		{
			nanogui::ref<EditorGUI> app = new EditorGUI();
			nanogui::Theme *myTheme = new nanogui::Theme(app->nvgContext());
			setupTheme(myTheme);
			app->setTheme(myTheme);
			app->createWindows();
			app->setVisible(true);

			nanogui::mainloop();
		}

		nanogui::shutdown();
	}
	catch (const std::runtime_error &e) {
		std::string error_msg = std::string("Caught a fatal error: ") + std::string(e.what());
#if defined(_WIN32)
		MessageBoxA(nullptr, error_msg.c_str(), NULL, MB_ICONERROR | MB_OK);
#else
		std::cerr << error_msg << endl;
#endif
		return -1;
	}

	return 0;
}

