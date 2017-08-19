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

#include <iostream>
#include <fstream>
#include <locale>
#include <string>
#include <vector>
#include <set>
#include <functional>
#include <regular_expressions.h>
#include "propertymonitor.h"
#include "draghandle.h"
#include "manuallayout.h"
#include "skeleton.h"
#include "lineplot.h"
#include "panelscreen.h"
#include "editorproject.h"
#include "editorsettings.h"
#include "editorgui.h"
#include "anchor.h"
#include "linkableproperty.h"

#include <libgen.h>
#include <zmq.hpp>
#include <cJSON.h>
#include <MessageEncoding.h>
#include <MessagingInterface.h>
#include <signal.h>
#include <SocketMonitor.h>
#include <ConnectionManager.h>
#include <circularbuffer.h>
#include <symboltable.h>
#include <Logger.h>
#include <DebugExtra.h>

#include <boost/thread.hpp>
#include <boost/thread/condition.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/program_options.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include "propertylist.h"
#include "gltexture.h"
#include "palette.h"
#include "selectablewidget.h"
#include "selectablebutton.h"
#include "userwindow.h"
#include "cJSON.h"
#include "viewlistcontroller.h"
#include "editorobject.h"
#include "linkableobject.h"
#include "editor.h"
#include "factorybuttons.h"
#include "editorwidget.h"
#include "editorbutton.h"
#include "editortextbox.h"
#include "editorlabel.h"
#include "editorimageview.h"
#include "editorlineplot.h"
#include "editorprogressbar.h"
#include "structureswindow.h"
#include "objectwindow.h"
#include "patternswindow.h"
#include "themewindow.h"
#include "propertywindow.h"
#include "helper.h"
#include "curl_helper.h"
#include "screenswindow.h"

// settings file parser globals
#define __MAIN__ 1
#include "settingslang.h"
SymbolTable globals;
std::list<Structure *>st_structures;
std::map<std::string, Structure *>structures;
extern FILE *st_yyin;
int st_yyparse();
extern int st_yycharno;
extern int st_yylineno;
const char *st_yyfilename = 0;

#include "hmilang.h"
extern std::list<Structure *>hm_structures;
extern std::list<StructureClass *> hm_classes;
extern FILE *yyin;
int yyparse();
extern int yycharno;
extern int yylineno;
const char *yyfilename = 0;
const char *filename = 0;
class LinkableProperty;
std::map<std::string, LinkableProperty *> remotes;

long full_screen_mode = 0;
int run_only = 0;

extern long collect_history;

int num_errors = 0;
std::list<std::string>error_messages;
std::list<std::string>settings_files;
std::list<std::string> source_files;

Structure *system_settings = 0;

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
std::string tag_file_name;

const char *program_name;

//extern int cw_out;
//extern std::string host;
extern const char *local_commands;
extern ProgramState program_state;
extern struct timeval start;

struct Texture {
	Texture(GLTexture tex, GLTexture::handleType dat) : texture( std::move(tex)), data(std::move(dat)) {}
	GLTexture texture;
	GLTexture::handleType data;
};
std::map<std::string, Texture*> texture_cache;

extern void setup_signals();

std::string stripEscapes(const std::string &s);
StructureClass *findClass(const std::string &name);

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

class Toolbar : public nanogui::Window {
public:
	Toolbar(EditorGUI *screen, nanogui::Theme *);
	nanogui::Window *getWindow() { return this; }
	bool mouseDragEvent(const Vector2i &p, const Vector2i &rel, int button, int modifiers) override {
		bool res = nanogui::Window::mouseDragEvent(p, rel, button, modifiers);
		updateSettingsStructure("ToolBar", this);
		return res;
	}
private:
	EditorGUI *gui;
};

class ListPanel : public nanogui::Widget, public Palette {
public:
	ListPanel(nanogui::Widget *owner);
	virtual ~ListPanel() {}
	void update();
	Selectable *getSelectedItem();
	void selectFirst();
	void select(const std::string path);
private:
	nanogui::VScrollPanel *palette_scroller;
	nanogui::Widget *palette_content;
};

class StartupWindow : public Skeleton {
public:
	StartupWindow(EditorGUI *screen, nanogui::Theme *theme);
	void setVisible(bool which) { window->setVisible(which); }

private:
	std::string message;
	EditorGUI *gui;
	ListPanel *project_box;
};

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

class PropertyFormHelper : public nanogui::FormHelper {
public:
	PropertyFormHelper(nanogui::Screen *screen) : nanogui::FormHelper(screen), mContent(0) { }
	void clear() {
		using namespace nanogui;
		while (window()->childCount()) {
			window()->removeChild(0);
		}
	}

	nanogui::Window *addWindow(const Vector2i &pos,
							  const std::string &title = "Untitled") override {
		assert(mScreen);
		if (mWindow) { mWindow->decRef(); mWindow = 0; }
		PropertyFormWindow *pfw = new PropertyFormWindow(mScreen, title);
		mWindow = pfw;
		mWindow->setSize(nanogui::Vector2i(320, 640));
		mWindow->setFixedSize(nanogui::Vector2i(320, 640));
		nanogui::VScrollPanel *palette_scroller = new nanogui::VScrollPanel(mWindow);
		palette_scroller->setSize(Vector2i(mWindow->width(), mWindow->height() - mWindow->theme()->mWindowHeaderHeight));
		palette_scroller->setFixedSize(Vector2i(mWindow->width(), mWindow->height() - mWindow->theme()->mWindowHeaderHeight));
		palette_scroller->setPosition( Vector2i(0, mWindow->theme()->mWindowHeaderHeight+1));
		mContent = new nanogui::Widget(palette_scroller);
		pfw->setContent(mContent);
		mContent->setFixedSize(Vector2i(palette_scroller->width()-20,  palette_scroller->height()));
		mLayout = new nanogui::AdvancedGridLayout({20, 0, 30, 0}, {});
		mLayout->setMargin(1);
		setFixedSize(Vector2i(150, 30));
		//mLayout->setColStretch(1, 0);
		mLayout->setColStretch(2, 1);
		mContent->setLayout(mLayout);
		mWindow->setPosition(pos);
		mWindow->setLayout( new nanogui::BoxLayout(nanogui::Orientation::Vertical) );
		mWindow->setVisible(true);
		return mWindow;
	}

	void setWindow(nanogui::Window *wind) override {
		assert(mScreen);
		mWindow = wind;
		mWindow->setSize(nanogui::Vector2i(320, 640));
		mWindow->setFixedSize(nanogui::Vector2i(320, 640));
		nanogui::VScrollPanel *palette_scroller = new nanogui::VScrollPanel(mWindow);
		palette_scroller->setSize(Vector2i(mWindow->width(), mWindow->height() - mWindow->theme()->mWindowHeaderHeight));
		palette_scroller->setFixedSize(Vector2i(mWindow->width(), mWindow->height() - mWindow->theme()->mWindowHeaderHeight));
		palette_scroller->setPosition( Vector2i(0, mWindow->theme()->mWindowHeaderHeight+1));
		mContent = new nanogui::Widget(palette_scroller);
		PropertyFormWindow *pfw = dynamic_cast<PropertyFormWindow*>(wind);
		if (pfw) pfw->setContent(mContent);
		mContent->setFixedSize(Vector2i(palette_scroller->width()-20,  palette_scroller->height()));
		mLayout = new nanogui::AdvancedGridLayout({20, 0, 30, 0}, {});
		mLayout->setMargin(1);
		setFixedSize(Vector2i(150, 30));
		//mLayout->setColStretch(1, 0);
		//mLayout->setColStretch(2, 1);
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

class ViewsWindow : public nanogui::Object {
public:
	ViewsWindow(EditorGUI *screen, nanogui::Theme *theme);
	void setVisible(bool which) { window->setVisible(which); }
	nanogui::Window *getWindow()  { return window; }
	void addWindows();
	void add(const std::string name, nanogui::Widget *);
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
	tb->setFlags(Button::NormalButton);
	tb->setTooltip("New Project");
	tb->setFixedSize(Vector2i(32,32));

	tb = new ToolButton(toolbar, ENTYPO_ICON_NOTE);
	//tb->setFlags(Button::ToggleButton);
	tb->setTooltip("Open Project");
	tb->setFlags(Button::NormalButton);
	tb->setCallback([this] {
		Editor *editor = EDITOR;
		if (editor) {
			std::string file_path(file_dialog(
				{ {"humid", "Humid layout file"} }, false));
			if (file_path.length()) {
				editor->load(file_path);
				editor->gui()->getScreensWindow()->update();
			}
		}
	});

	tb = new ToolButton(toolbar, ENTYPO_ICON_SAVE);
	//tb->setFlags(Button::ToggleButton);
	tb->setTooltip("Save Project");
	tb->setFlags(Button::NormalButton);
	tb->setCallback([this] {
		Editor *editor = EDITOR;
		if (editor) {
			Structure *es = EditorSettings::find("EditorSettings");
			if (!es) es = EditorSettings::create();
			const Value &base_v(es->getProperties().find("project_base"));
			if (base_v == SymbolTable::Null) {
				std::string file_path(file_dialog(
					{ {"humid", "Humid layout file"},
					{"txt", "Text file"} }, true));
				if (file_path.length()) {
					editor->saveAs(file_path);
					editor->gui()->updateProperties();
					Structure *s = editor->gui()->getSettings();
					boost::filesystem::path base(source_files.front());
					if (boost::filesystem::is_regular_file(base))
						base = base.parent_path();
					assert(boost::filesystem::is_directory(base));
					s->getProperties().add("project_base", Value(base.string(), Value::t_string));
					EditorSettings::setDirty(); // TBD fix this
					EditorSettings::flush();
				}
			}
			else {
				editor->save();
			}
		}
	});
	tb->setFixedSize(Vector2i(32,32));

	tb = new ToolButton(toolbar, ENTYPO_ICON_OPEN_BOOK);
	tb->setFlags(Button::ToggleButton);
	tb->setTooltip("Tags");
	tb->setFixedSize(Vector2i(32,32));
	tb->setCallback([&] {
		std::string tags(file_dialog(
		  { {"csv", "Clockwork TAG file"}, {"txt", "Text file"} }, false));
		gui->getObjectWindow()->loadTagFile(tags);
	});

	tb = new ToolButton(toolbar, ENTYPO_ICON_INSTALL);
	tb->setTooltip("Refresh");
	tb->setFixedSize(Vector2i(32,32));
	tb->setChangeCallback([this](bool state) {
	});

	ToolButton *settings_button = new ToolButton(toolbar, ENTYPO_ICON_COG);
	settings_button->setFixedSize(Vector2i(32,32));
	settings_button->setTooltip("Theme properties");
	settings_button->setChangeCallback([this](bool state) {
		this->gui->getThemeWindow()->setVisible(state);
		if (state)
			this->gui->getThemeWindow()->getWindow()->requestFocus();
	});

	tb = new ToolButton(toolbar, ENTYPO_ICON_LAYOUT);
	tb->setTooltip("Create");
	tb->setFixedSize(Vector2i(32,32));
	tb->setChangeCallback([](bool state) { });

	tb = new ToolButton(toolbar, ENTYPO_ICON_EYE);
	tb->setTooltip("Views");
	tb->setFixedSize(Vector2i(32,32));
	tb->setChangeCallback([this](bool state) {
		this->gui->getViewsWindow()->setVisible(state);
		this->gui->getViewsWindow()->getWindow()->requestFocus();
 	});

	BoxLayout *bl = new BoxLayout(Orientation::Horizontal);
	toolbar->setLayout(bl);
	toolbar->setTitle("Toolbar");

	toolbar->setVisible(false);
}

void ObjectWindow::rebuildWindow() {
	std::cout << "rebuilding object window\n";
	const std::map<std::string, LinkableProperty*> &properties(gui->getLinkableProperties());
	std::set<std::string>groups;
	for (auto item : properties) {
		groups.insert(item.second->group());
	}
	for (auto group : groups) {
		std::cout << "createing object window tab: " << group << "\n";
		createTab(group.c_str());
		if (boost::filesystem::exists(group)) {// load a tag file if given
			std::cout << "tag file found, loading: " << group << " from file\n";
			createPanelPage(group.c_str(), gui->getObjectWindow()->getPaletteContent());
		}
		loadItems(group, search_box->value());
	}
	//gui->refreshData();
}

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
void ListPanel::select(const std::string path) {

}

class ScreenProjectButton : public SelectableButton {
	public:
	ScreenProjectButton(const std::string kind, Palette *pal, nanogui::Widget *parent,
			const std::string &caption)
		: SelectableButton(kind, pal, parent, caption), project(0) { }
	void setProject( Structure *s) { project = s; }
	Structure *getScreen() { return project; }
	private:
	Structure *project;
};


StartupWindow::StartupWindow(EditorGUI *screen, nanogui::Theme *theme) : Skeleton(screen), gui(screen) {
	using namespace nanogui;
	window->setTheme(theme);
	Label *itemText = new Label(window, "", "sans-bold");
	itemText->setPosition(Vector2i(40, 40));
	itemText->setSize(Vector2i(260, 20));
	itemText->setFixedSize(Vector2i(260, 20));
	itemText->setCaption("Select a project to open");
	itemText->setVisible(true);

	project_box = new ListPanel(window);
	project_box->setSize(Vector2i(300, 100));
	project_box->setPosition(Vector2i(50,100));
	project_box->setLayout(new BoxLayout(Orientation::Vertical, Alignment::Fill));

	Button *newProjectButton = new Button(project_box, "New");
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

class UserWindowWin : public SkeletonWindow, public EditorObject {
public:
	UserWindowWin(EditorGUI *s, const std::string caption) : SkeletonWindow(s, caption), EditorObject(0), gui(s), current_item(-1) {
	}

	bool keyboardEvent(int key, int /* scancode */, int action, int modifiers) override;

	bool mouseEnterEvent(const Vector2i &p, bool enter) override;

	virtual bool mouseButtonEvent(const nanogui::Vector2i &p, int button, bool down, int modifiers) override {
		if (button == GLFW_MOUSE_BUTTON_RIGHT && !down) return true;
		return SkeletonWindow::mouseButtonEvent(p, button, down, modifiers);
	}

	void update();

	virtual void draw(NVGcontext *ctx) override;

	void setCurrentItem(int n) { current_item = n; }
	int currentItem() { return current_item; }

private:
	EditorGUI *gui;
	int current_item;
};

void UserWindowWin::draw(NVGcontext *ctx) {
	nvgSave(ctx);
//    glMatrixMode(GL_PROJECTION);
//    glLoadIdentity();
//	float aspect = (float)width() / (float)height();
//    glOrtho(3.0 * aspect, 3.0 * aspect, -3.0, 3.0, 1.0, 50.0);
//	glMatrixMode(GL_MODELVIEW);
//    glLoadIdentity();
//nvgScale(ctx, 0.9f, 0.9f);
	nanogui::Window::draw(ctx);
	//nvgScale(ctx, 1.0f/0.9f, 1.0f/1.9f);
	nvgRestore(ctx);

}

bool UserWindowWin::keyboardEvent(int key, int scancode , int action, int modifiers) {
	if (action == GLFW_PRESS || action == GLFW_REPEAT) {
		if (EDITOR && EDITOR->isEditMode()) {
			UserWindow *uw = gui->getUserWindow();
			if (action == GLFW_PRESS && (key == GLFW_KEY_BACKSPACE || key == GLFW_KEY_DELETE) ) {
				if (uw && uw->hasSelections()) {
					uw->deleteSelections();
					return false;
				}
			}
			else if ( action == GLFW_PRESS && key == GLFW_KEY_TAB && childCount() > 1) {
				bool backward = modifiers & GLFW_MOD_SHIFT;
				int start = current_item;
				int n = childCount();
				int i = start;
				int maxpos = -1;
				int minpos = -1;
				int maxidx = start;
				int minidx = start;
				int start_pos = -1;
				if (start >= 0) {
					nanogui::Widget *w = childAt(start);
					EditorWidget *ew = dynamic_cast<EditorWidget*>(childAt(start));
					if (ew) { start_pos = ew->tabPosition(); }
				}


				// search for a widget with a lower tab position
				while (--i != start) {
					if (i < 0) i = n-1;
					if (i == start) break;
					nanogui::Widget *w = childAt(i);
					EditorWidget *ew = dynamic_cast<EditorWidget*>(w);
					if (!ew) continue;
					int pos = ew->tabPosition();
					if (pos>0) {
						if (backward) {
							if (pos > minpos && (start_pos == -1 || pos <= start_pos) ) { minpos = pos; minidx = i; }
							if ( (pos > maxpos || maxpos == -1) && pos >= start_pos ) { maxpos = pos; maxidx = i; }
						}
						else {
							if ( (pos < minpos || minpos == -1) && (start_pos == -1 || pos <= start_pos) ) { minpos = pos; minidx = i; }
							if ( (pos < maxpos || maxpos == -1) && pos >= start_pos ) { maxpos = pos; maxidx = i; }
						}
					}
				}
				int new_sel = current_item;
				if (backward) {
					if (minidx != start) new_sel = minidx; else if (maxidx != start) new_sel = maxidx;
				}
				else {
					if (maxidx != start) new_sel = maxidx; else if (minidx != start) new_sel = minidx;
				}
				if (new_sel >= 0 && new_sel != start) {
					EditorWidget *ew = 0;
					if (start >= 0) ew = dynamic_cast<EditorWidget*>(childAt(start));
					if (ew) ew->deselect();
					ew = dynamic_cast<EditorWidget*>(childAt(new_sel));
					if (ew) ew->select();
				}
			}
			else if ( uw->hasSelections() &&
						( key == GLFW_KEY_LEFT || key == GLFW_KEY_UP || key == GLFW_KEY_RIGHT || key == GLFW_KEY_DOWN ) ) {
				if (EDITOR->getDragHandle()) EDITOR->getDragHandle()->setVisible(false);
				for (auto item : uw->getSelected() ) {
					nanogui::Widget *w = dynamic_cast<nanogui::Widget*>(item);
					if (!w) continue;
					switch (key) {
						case GLFW_KEY_LEFT:
							if (w->position().x()> 0)
								w->setPosition( nanogui::Vector2i(w->position().x()-1, w->position().y()) );
							break;
						case GLFW_KEY_RIGHT:
							if (w->position().x() + w->width() < w->parent()->width())
								w->setPosition( nanogui::Vector2i(w->position().x()+1, w->position().y()) );
							break;
						case GLFW_KEY_UP:
							if (w->position().y()> w->parent()->theme()->mWindowHeaderHeight)
								w->setPosition( nanogui::Vector2i(w->position().x(), w->position().y()-1) );
							break;
						case GLFW_KEY_DOWN:
							if (w->position().y() + w->height() < w->parent()->height())
								w->setPosition( nanogui::Vector2i(w->position().x(), w->position().y()+1) );
							break;
						default:
							break;
					}
				}
			}
		}
		bool handled = false;
		for (auto item : children() ) {
			nanogui::LinePlot *lp = dynamic_cast<nanogui::LinePlot*>(item);
			if (lp && lp->focused()) { lp->handleKey(key, scancode, action, modifiers); handled = true; }
		}
		if (!handled) return Window::keyboardEvent(key, scancode, action, modifiers);
	}
	return true;
}

bool UserWindowWin::mouseEnterEvent(const Vector2i &p, bool enter) {
	bool res = Window::mouseEnterEvent(p, enter);
	StructuresWindow *sw = EDITOR->gui()->getStructuresWindow();;
	ObjectWindow *ow = EDITOR->gui()->getObjectWindow();
	if (!focused() && enter && EDITOR->isEditMode() && ( (sw && sw->hasSelections()) || (ow && ow->hasSelections() )))
		requestFocus();
	//else if (focused() && !enter)
	//		setFocused(false);

	return res;
}

UserWindow::UserWindow(EditorGUI *screen, nanogui::Theme *theme, UserWindowWin *uww)
: Skeleton(screen, uww), LinkableObject(0), gui(screen), current_layer(0), mDefaultSize(1024,768), current_structure(0) {
	using namespace nanogui;
	gui = screen;
	window->setTheme(theme);
	GLFWmonitor* primary = glfwGetPrimaryMonitor();
	const GLFWvidmode* mode = glfwGetVideoMode(primary);
	mDefaultSize.x() = mode->width;
	mDefaultSize.y() = mode->height;
	window->setFixedSize(mDefaultSize);
	window->setSize(mDefaultSize);
	window->setVisible(false);
	window->setTitle("");
	window->setPosition(nanogui::Vector2i(0,0));
	push(window);
}

void UserWindow::update(const Value &value) {
	if (!EDITOR->isEditMode()) {
		Structure *s = findScreen(value.asString());
		if (s)
			system_settings->getProperties().add("active_screen", Value(s->getName(), Value::t_string));
	}
}

void UserWindow::select(Selectable * w) {
	if (!dynamic_cast<nanogui::Widget*>(w)) return;
	Palette::select(w);
	UserWindowWin *wnd = dynamic_cast<UserWindowWin*>(window);
	nanogui::Widget *widget = dynamic_cast<nanogui::Widget *>(w);
	if (widget && wnd) wnd->setCurrentItem(window->childIndex(widget));
}
void UserWindow::deselect(Selectable *w) {
	if (!dynamic_cast<nanogui::Widget*>(w)) return;
	Palette::deselect(w);
	UserWindowWin *wnd = dynamic_cast<UserWindowWin*>(window);
	if (wnd) wnd->setCurrentItem(-1);
}

void UserWindow::fixLinks(LinkableProperty *lp) {
	int n = window->childCount();
	int idx = 0;
	while (n--) {
		EditorWidget *ew = dynamic_cast<EditorWidget *>(window->childAt(idx));
		if (ew->getDefinition()) {
			const Value &r = ew->getDefinition()->getProperties().find("Remote");
			if (r != SymbolTable::Null && r.asString() == lp->tagName()) {
				if (ew->getRemote()) ew->getRemote()->unlink(ew->getRemote());
				ew->setRemote(lp);
				ew->setProperty("Remote", lp->tagName());
			}
		}
	}
}

void UserWindow::setStructure( Structure *s) {
	if (!s) return;

	StructureClass *sc = findClass(s->getKind());
	if ( s->getKind() != "SCREEN" && (!sc || sc->getBase() != "SCREEN") ) return;

	// save current settings
	if (structure()) {
		std::list<std::string>props;
		std::map<std::string, std::string> property_map;
		loadPropertyToStructureMap(property_map);
		getPropertyNames(props);
		for (auto prop : props) {
			Value v = getPropertyValue(prop);
			if (v != SymbolTable::Null) {
				auto item = property_map.find(prop);
				if (item != property_map.end())
					structure()->getProperties().add((*item).second, v);
				else
					structure()->getProperties().add(prop, v);
			}
		}
	}

	clearSelections();
	nanogui::DragHandle *drag_handle = EDITOR->getDragHandle();
	PropertyMonitor *pm  = nullptr;
	if (drag_handle) {
		drag_handle->incRef();
		window->removeChild(drag_handle);
		pm = drag_handle->propertyMonitor();
		drag_handle->setPropertyMonitor(0);
	}
	int n = window->childCount();
	while (n--) {
		window->removeChild(0);
	}

	loadStructure(s);
	current_structure = s;

  window->addChild(drag_handle);
	if (drag_handle) {
	  drag_handle->setPropertyMonitor(pm);
	  drag_handle->decRef();
	}
  gui->performLayout();
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

void UserWindow::clear() {
	nanogui::DragHandle *drag_handle = EDITOR->getDragHandle();
	if (drag_handle) drag_handle->setVisible(false);
	drag_handle->incRef();
	window->removeChild(drag_handle);
	PropertyMonitor *pm = drag_handle->propertyMonitor();
	drag_handle->setPropertyMonitor(0);

	int n = window->childCount();
	int idx = 0;
	while (n--) {
		NamedObject *no = dynamic_cast<NamedObject *>(window->childAt(idx));
		window->removeChild(idx);
	}
	window->addChild(drag_handle);

	drag_handle->setPropertyMonitor(pm);
	drag_handle->decRef();

	window->performLayout( gui->nvgContext() );
}

CircularBuffer *UserWindow::getValues(const std::string name) {
	auto iter = data.find(name);
	if (iter != data.end()) {
		const std::pair<std::string, CircularBuffer *> &node = *iter;
		return node.second;
	}
	return 0;
}

CircularBuffer * UserWindow::addDataBuffer(const std::string name, CircularBuffer::DataType dt, size_t len) {
	auto found = data.find(name);
	if (found != data.end()) {
		CircularBuffer *buf = (*found).second;
		if (buf->getDataType() == dt) return buf;
		CircularBuffer *new_buf = new CircularBuffer(len, dt);
		data[name] = new_buf;
		delete buf;
		return new_buf;
	}
	else {
		CircularBuffer *buf = new CircularBuffer(len, dt);
		data[name] = buf;
		return buf;
  }
}

// TBD remove this hack (see lineplot)
CircularBuffer *UserWindow::createBuffer(const std::string name) {
	CircularBuffer *res = getValues(name);
	if (!res) {
		LinkableProperty *lp = gui->findLinkableProperty(name);
		if (lp)
			res = addDataBuffer(name, lp->dataType(), gui->sampleBufferSize());
	}
	return res;
}

void loadProjectFiles(std::list<std::string> &files_and_directories) {
	using namespace boost::filesystem;

	Structure *settings = EditorSettings::find("EditorSettings");
	if (!settings) settings = EditorSettings::create();
	assert(settings);
	bool base_checked = false;

	std::list<path> files;
	{
		std::list<std::string>::iterator fd_iter = files_and_directories.begin();
		while (fd_iter != files_and_directories.end()) {
			path fp = (*fd_iter++).c_str();
			if (!exists(fp)) continue;
			if (is_regular_file(fp)) {
				std::string ext = boost::filesystem::extension(fp);
			 	if (ext == ".humid") files.push_back(fp);
			}
			else if (is_directory(fp)) {
				if (!base_checked) {
					base_checked = true;
					settings->getProperties().add("project_base", Value(fp.string().c_str(), Value::t_string));
					EditorSettings::setDirty();
					EditorSettings::flush();
				}
				collect_humid_files(fp, files);
			}
		}
	}

	std::string base = "";
	if (settings) base = settings->getProperties().find("project_base").asString();
	assert(boost::filesystem::is_directory(base));
	std::cout << "Project Base: " << base << "\n";

	/* load configuration from files named on the commandline */
	int opened_file = 0;
	std::set<std::string>loaded_files;
	std::list<path>::const_iterator f_iter = files.begin();
	while (f_iter != files.end())
	{
		const char *filename = (*f_iter).string().c_str();
		std::string fname(filename);

		if (filename[0] != '-')
		{
			// strip project path from the file name
			if (base.length() && fname.find(base) == 0) {
				fname = fname.substr(base.length()+1);
				if (loaded_files.count(fname)) {
					std::cout << "Skipping second load of " << fname << "\n";
					continue; // already loaded this one-line
				}
				loaded_files.insert(fname);
			}
			if (exists(filename)) {
				std::cout << "reading project file " << fname << "\n";
				opened_file = 1;
				yyin = fopen(filename, "r");
				if (yyin)
				{
					std::cerr << "Processing file: " << filename << "\n";
					yylineno = 1;
					yycharno = 1;
					yyfilename = fname.c_str();
					yyparse();
					fclose(yyin);
				}
				else
				{
					std::stringstream ss;
					ss << "## - Error: failed to load project file: " << filename;
					error_messages.push_back(ss.str());
					++num_errors;
				}
			}
		}
		else if (strlen(filename) == 1) /* '-' means stdin */
		{
			opened_file = 1;
			std::cerr << "\nProcessing stdin\n";
			yyfilename = "stdin";
			yyin = stdin;
			yylineno = 1;
			yycharno = 1;
			yyparse();
		}
		f_iter++;
	}
}

void fixElementPosition(nanogui::Widget *w, const SymbolTable &properties) {
	Value vx = properties.find("pos_x");
	Value vy = properties.find("pos_y");
	if (vx != SymbolTable::Null && vx != SymbolTable::Null) {
		long x, y;
		if (vx.asInteger(x) && vy.asInteger(y)) w->setPosition(nanogui::Vector2i(x,y));
	}
}

void fixElementSize(nanogui::Widget *w, const SymbolTable &properties) {
	Value vx = properties.find("width");
	Value vy = properties.find("height");
	if (vx != SymbolTable::Null && vx != SymbolTable::Null) {
		long x, y;
		if (vx.asInteger(x) && vy.asInteger(y)) {
			w->setSize(nanogui::Vector2i(x,y));
			w->setFixedSize(nanogui::Vector2i(x,y));
		}
	}
}

void UserWindow::loadStructure( Structure *s) {
	StructureClass *sc = findClass(s->getKind());
	if (sc && (s->getKind() == "SCREEN" || sc->getBase() == "SCREEN") ) {
		if (sc && !s->getStructureDefinition())
			s->setStructureDefinition(sc);
		int pnum = 0;
		for (auto param : sc->getLocals()) {
			++pnum;
			Structure *element = param.machine;
			if (!element) {
				std::cout << "Warning: no structure for parameter " << pnum << "of " << s->getName() << "\n";
				continue;
			}
			std::string kind = element->getKind();
			StructureClass *element_class = findClass(kind);
			const Value &remote(element->getProperties().find("remote"));
			const Value &connection(element->getProperties().find("connection"));
			const Value &font_size_val(element->getProperties().find("font_size"));
			long font_size = 0;
			if (font_size_val != SymbolTable::Null) font_size_val.asInteger(font_size);
			const Value &scale_val(element->getProperties().find("value_scale"));
			double value_scale = 1.0f;
			if (scale_val != SymbolTable::Null) scale_val.asFloat(value_scale);
			long tab_pos = 0;
			const Value &tab_pos_val(element->getProperties().find("tab_pos"));
			if (tab_pos_val != SymbolTable::Null) tab_pos_val.asInteger(tab_pos);
			LinkableProperty *lp = nullptr;
			if (remote != SymbolTable::Null)
				lp = gui->findLinkableProperty(remote.asString());
			if (kind == "LABEL") {
				const Value &caption_v(element->getProperties().find("caption"));
				EditorLabel *el = new EditorLabel(s, window, element->getName(), lp,
												  (caption_v != SymbolTable::Null)?caption_v.asString(): element->getName());
				el->setName(element->getName());
				el->setDefinition(element);
				fixElementPosition( el, element->getProperties());
				fixElementSize( el, element->getProperties());
				if (connection != SymbolTable::Null) {
					el->setConnection(connection.asString());
				}
				if (font_size) el->setFontSize(font_size);
				if (value_scale != 1.0) el->setValueScale( value_scale );
				if (tab_pos) el->setTabPosition(tab_pos);
				if (lp)
					lp->link(new LinkableText(el));
				el->setChanged(false);
			}
			if (kind == "IMAGE") {
				const Value &image_file_v(element->getProperties().find("image_file"));
				EditorImageView *el = new EditorImageView(s, window, element->getName(), lp, 0);
				el->setName(element->getName());
				el->setDefinition(element);
				if (connection != SymbolTable::Null) {
					el->setConnection(connection.asString());
				}
				const Value &img_scale_val(element->getProperties().find("scale"));
				double img_scale = 1.0f;
				if (img_scale_val != SymbolTable::Null) img_scale_val.asFloat(img_scale);
				fixElementPosition( el, element->getProperties());
				fixElementSize( el, element->getProperties());
				if (font_size) el->setFontSize(font_size);
				if (value_scale != 1.0) el->setValueScale( value_scale );
				el->setScale( img_scale );
				if (tab_pos) el->setTabPosition(tab_pos);
				if (image_file_v != SymbolTable::Null) {
					std::string ifn = image_file_v.asString();
					el->setImageName(ifn);
					el->fit();
				}
				if (lp) {
					lp->link(new LinkableText(el));
				}
				el->setChanged(false);
			}
			if (kind == "PROGRESS") {
				EditorProgressBar *ep = new EditorProgressBar(s, window, element->getName(), lp);
				ep->setDefinition(element);
				fixElementPosition( ep, element->getProperties());
				fixElementSize( ep, element->getProperties());
				if (connection != SymbolTable::Null) {
					ep->setConnection(connection.asString());
				}
				if (value_scale != 1.0) ep->setValueScale( value_scale );
				if (tab_pos) ep->setTabPosition(tab_pos);
				ep->setChanged(false);
				if (lp)
					lp->link(new LinkableNumber(ep));
			}
			if (kind == "TEXT") {
				EditorTextBox *textBox = new EditorTextBox(s, window, element->getName(), lp);
				textBox->setDefinition(element);
				const Value &text_v(element->getProperties().find("text"));
				if (text_v != SymbolTable::Null) textBox->setValue(text_v.asString());
				const Value &alignment_v(element->getProperties().find("alignment"));
				if (alignment_v != SymbolTable::Null) textBox->setPropertyValue("Alignment", alignment_v.asString());
				textBox->setEnabled(true);
				textBox->setEditable(true);
				if (connection != SymbolTable::Null) {
					textBox->setConnection(connection.asString());
				}
				if (value_scale != 1.0) textBox->setValueScale( value_scale );
				fixElementPosition( textBox, element->getProperties());
				fixElementSize( textBox, element->getProperties());
				if (font_size) textBox->setFontSize(font_size);
				if (tab_pos) textBox->setTabPosition(tab_pos);
				textBox->setName(element->getName());
				if (lp)
					textBox->setTooltip(remote.asString());
				else
					textBox->setTooltip(element->getName());
				EditorGUI *gui = this->gui;
				if (lp)
					lp->link(new LinkableText(textBox));
				textBox->setChanged(false);
				textBox->setCallback( [textBox, gui](const std::string &value)->bool{
					if (!textBox->getRemote()) return true;
					const std::string &conn = textBox->getRemote()->group();
					char *rest = 0;
					{
						long val = strtol(value.c_str(),&rest,10);
						if (*rest == 0) {
							gui->queueMessage(conn,
									gui->getIODSyncCommand(conn,
									textBox->getRemote()->address_group(),
									textBox->getRemote()->address(), (int)val),
								[](std::string s){std::cout << s << "\n"; });
							return true;
						}
					}
					{
						double val = strtod(value.c_str(),&rest);
						if (*rest == 0)  {
							gui->queueMessage(conn,
								gui->getIODSyncCommand(conn, textBox->getRemote()->address_group(),
									textBox->getRemote()->address(), (float)val), [](std::string s){std::cout << s << "\n"; });
							return true;
						}
					}
					return false;
				});
			}
			else if (kind == "PLOT" || (element_class &&element_class->getBase() == "PLOT") ) {
				EditorLinePlot *lp = new EditorLinePlot(s, window, element->getName(), nullptr);
				lp->setDefinition(element);
				lp->setBufferSize(gui->sampleBufferSize());
				fixElementPosition( lp, element->getProperties());
				fixElementSize( lp, element->getProperties());
				if (connection != SymbolTable::Null) {
					lp->setConnection(connection.asString());
				}
				if (value_scale != 1.0) lp->setValueScale( value_scale );
				if (font_size) lp->setFontSize(font_size);
				if (tab_pos) lp->setTabPosition(tab_pos);
				if (element->getProperties().find("overlay").asString() == "1") lp->overlay(true);
				const Value &monitors(element->getProperties().find("monitors"));
				if (monitors != SymbolTable::Null) {
					lp->setMonitors(this, monitors.asString());
				}
				lp->setChanged(false);
			}
			else if (kind == "BUTTON" || kind == "INDICATOR") {
				const Value &caption_v(element->getProperties().find("caption"));
				EditorButton *b = new EditorButton(s, window, element->getName(), lp,
												   (caption_v != SymbolTable::Null)?caption_v.asString(): element->getName());
				if (kind == "INDICATOR") b->setEnabled(false); else b->setEnabled(true);
				b->setDefinition(element);
				b->setBackgroundColor(colourFromProperty(element, "bg_color"));
				b->setTextColor(colourFromProperty(element, "text_colour"));
				b->setOnColor(colourFromProperty(element, "bg_on_color"));
				b->setOnTextColor(colourFromProperty(element, "on_text_colour"));
				b->setFlags(element->getIntProperty("behaviour", nanogui::Button::NormalButton));
				if (connection != SymbolTable::Null) {
					b->setConnection(connection.asString());
				}
				fixElementPosition( b, element->getProperties());
				fixElementSize( b, element->getProperties());
				if (font_size) b->setFontSize(font_size);
				if (tab_pos) b->setTabPosition(tab_pos);
				EditorGUI *gui = this->gui;

				{
					const Value &caption_v = element->getProperties().find("caption");
					if (caption_v != SymbolTable::Null) b->setCaption(caption_v.asString());
				}
				{
					const Value &caption_v = element->getProperties().find("on_caption");
					if (caption_v != SymbolTable::Null) b->setOnCaption(caption_v.asString());
				}
				{
					const Value &cmd(element->getProperties().find("command"));
					if (cmd != SymbolTable::Null && cmd.asString().length()) b->setCommand(cmd.asString());
				}
				b->setupButtonCallbacks(lp, gui);
				b->setChanged(false);
			}
		}
	}
	if (s->getKind() == "REMOTE") {
		const Value &rname(s->getProperties().find("NAME"));
		if (rname != SymbolTable::Null) {
			LinkableProperty *lp = gui->findLinkableProperty(rname.asString());
			if (lp) {
				remotes[s->getName()] = lp;
			}
		}
	}
	if (s->getKind() == "SCREEN" || (sc && sc->getBase() == "SCREEN") ) {
		const Value &title(s->getProperties().find("caption"));
		//if (title != SymbolTable::Null) window->setTitle(title.asString());
		PanelScreen *ps = getActivePanel();
		if (ps) ps->setName(s->getName());

		ThemeWindow *tw = EDITOR->gui()->getThemeWindow();
		nanogui::Widget *w = dynamic_cast<nanogui::Widget*>(EDITOR->gui()->getUserWindow()->getWindow());
		if (w && tw && tw->getWindow()->visible() ) {
			tw->loadTheme(w->theme());
		}
		PropertyWindow *prop = EDITOR->gui()->getPropertyWindow();
		if (prop && prop->getWindow()->visible()) {
			if (!EDITOR->isEditMode())
				prop->getWindow()->setVisible(false);
			else
				prop->update();
			EDITOR->gui()->needsUpdate();
		}
		s->setChanged(false);
	}
}

void UserWindow::load(const std::string &path) {
	std::list<std::string> files;
	files.push_back(path);
	loadProjectFiles(files);
}

void UserWindow::save(const std::string &path) {
	using namespace nanogui;
	std::ofstream out(path);

	{
		const std::map<std::string, LinkableProperty*> &properties(gui->getLinkableProperties());
		std::set<std::string>groups;
		for (auto item : properties) {
			groups.insert(item.second->group());
		}
		for (auto group : groups) {
			out << shortName(group) << " CONNECTION_GROUP (path:\""<< group << "\");\n";
		}
	}
	std::stringstream pending_definitions;
	std::set<LinkableProperty*> used_properties;
	for (auto screen : gui->getScreens()) {
		std::string screen_type(screen->getName());
		boost::to_upper(screen_type);
		out << screen_type << " STRUCTURE EXTENDS SCREEN {\n"
			<< "  OPTION caption \"Untitled\";\n";
		for (auto it = window->children().rbegin(); it != window->children().rend(); ++it) {
			Widget *child = *it;
			{
			EditorButton *b = dynamic_cast<EditorButton*>(child);
			if (b) {
				out << b->getName() << " " << b->baseName() << " ("
					<< "pos_x: " << b->position().x() << ", pos_y: " << b->position().y()
					<< ", width: " << b->width() << ", height: " << b->height()
					<< ", caption: \"" << b->caption() << '"'
					<< ", font_size: " << b->fontSize()
					<< ", tab_pos: " << b->tabPosition();
				const nanogui::Color color = b->backgroundColor();
				out << ", bg_color: \"" << color.r() <<"," << color.g()<<","<<color.b()<<","<<color.w()<<"\"";
				if (b->command().length()) out << ", command: " << escapeQuotes(b->command());
				if (b->getRemote()) {
					used_properties.insert(b->getRemote());
					out << ", remote: \"" << b->getRemote()->tagName() << "\"";
				}
				out << ");\n";
				continue;
			}
			}
			{
			EditorLabel *el = dynamic_cast<EditorLabel*>(child);
			if (el) {
				out << el->getName() << " " << el->baseName() << " ("
					<< "pos_x: " << el->position().x()
					<< ", pos_y: " << el->position().y()
					<< ", width: " << el->width() << ", height: " << el->height()
					<< ", caption: \"" << el->caption() << '"'
					<< ", font_size: " << el->fontSize()
					<< ", tab_pos: " << el->tabPosition();
				if (el->getRemote()) {
					used_properties.insert(el->getRemote());
					out << ", remote: \"" << el->getRemote()->tagName() << "\"";
				}
				out << ");\n";
				continue;
			}
			}

			{
			EditorProgressBar *eb = dynamic_cast<EditorProgressBar*>(child);
			if (eb) {
				out << eb->getName() << " " << eb->baseName() << " ("
					<< "pos_x: " << eb->position().x()
					<< ", pos_y: " << eb->position().y()
					<< ", width: " << eb->width() << ", height: " << eb->height()
					<< ", tab_pos: " << eb->tabPosition();
				if (eb->getRemote()) {
					used_properties.insert(eb->getRemote());
					out << ", remote:\"" << eb->getRemote()->tagName() << "\"";
				}
				out << ");\n";
				continue;
			}
			}


			{
			EditorTextBox *t = dynamic_cast<EditorTextBox*>(child);
			if (t) {
				out << t->getName() << " " << t->baseName() << " ("
				<< "pos_x: " << t->position().x() << ", pos_y: " << t->position().y()
				<< ", width: " << t->width() << ", height: " << t->height()
				<< ", font_size: " << t->fontSize()
				<< ", tab_pos: " << t->tabPosition();
				if (!t->getRemote())
					out << ", value: " << escapeQuotes(t->value());
				/*else if (t->getRemote() && t->getRemote()->dataType() == CircularBuffer::STR)
					out << ", value: " << escapeQuotes(t->value());
				else
					out << ", value: " << t->value();*/
				if (t->getRemote()) {
					used_properties.insert(t->getRemote());
					out << ", remote: \"" << t->getRemote()->tagName() << "\"";
				}
				out << ");\n";
				continue;
			}
			}
			{
			EditorImageView *ip = dynamic_cast<EditorImageView*>(child);
			if (ip) {
				out << ip->getName() << " " << ip->baseName() << " ("
				<< "pos_x: " << ip->position().x() << ", pos_y: " << ip->position().y()
				<< ", width: " << ip->width() << ", height: " << ip->height()
				<< ", location: \"" << ip->imageName() << '"'
				<< ", tab_pos: " << ip->tabPosition();
				if (ip->getRemote()) {
					used_properties.insert(ip->getRemote());
					out << ", remote: \"" << ip->getRemote()->tagName() << "\"";
				}
				out <<  ");\n";
				continue;
			}
			}
			{
			EditorLinePlot *lp = dynamic_cast<EditorLinePlot*>(child);
			if (lp) {
				if (lp->getName().length() == 0)
					lp->setName(NamedObject::nextName(lp));
				out << lp->getName() << " " << lp->baseName()<< "_" << lp->getName()
					<< " ("
					<< "pos_x: " << lp->position().x() << ", pos_y: " << lp->position().y()
					<< ", width: " << lp->width() << ", height: " << lp->height()
					<< ", tab_pos: " << lp->tabPosition()
					<< ", x_scale: " << lp->xScale()
					<< ", grid_intensity: " << lp->gridIntensity()
					<< ", display_grid: " << lp->displayGrid()
					<< ", monitors: \"" << lp->monitors() << '"'
					<< ", overlay: " << lp->overlaid()
					<< ");\n";
				pending_definitions <<  lp->baseName() << "_" << lp->getName() << " STRUCTURE EXTENDS " << lp->baseName() << " {\n"
				<< "\tOPTION pos_x 50;\n"
				<< "\tOPTION pos_y 100;\n"
				<< "\tOPTION width 200;\n"
				<< "\tOPTION height 100;\n"
				<< "\tOPTION x_scale 1.0;\n"
				<< "\tOPTION grid_intensity 0.05;\n"
				<< "\tOPTION display_grid TRUE;\n\n";
				for (auto series : lp->getSeries()) {
					LinkableProperty *lp = gui->findLinkableProperty(series->getName());
					if (lp) used_properties.insert(lp);
					pending_definitions << "\t" << series->getName() << " SERIES (remote:" <<  series->getName() << ");\n";
				}
				pending_definitions << "}\n";
				continue;
			}
			}
		}
		PanelScreen *ps = getActivePanel();
		out << "}\n" << ps->getName() << " " << screen_type
			<< " (caption: \"" << escapeQuotes(window->title()) << "\");\n\n";
		out << pending_definitions.str() << "\n";

		for (auto link : used_properties) {
			out << link->tagName() << " REMOTE (";
			link->save(out);
			out << ");\n";

		}

	}
}

void UserWindow::update() {
	UserWindowWin * w = dynamic_cast<UserWindowWin*>( getWindow() );
	if (w) w->update();
}

void UserWindowWin::update() {
	// if the window contains a line plot, that object should be updated
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
		Structure *s = new Structure(nullptr, "Start_Button", "BUTTON");
		starters["BUTTON"] = s;
		StructureClass *sc = new StructureClass("BUTTON", "");
		sc->setBuiltIn();
		hm_classes.push_back(sc);

		s->getProperties().add("width",120);
		s->getProperties().add("height",60);

		cell = new Widget(palette_content);
		cell->setFixedSize(Vector2i(button_width+4,35));
		b = new StructureFactoryButton(gui, "INDICATOR", this, cell, 0, "INDICATOR", "");
		b->setEnabled(true);
		b->setFixedSize(Vector2i(button_width, 30));
		b->setPosition(Vector2i(2,2));
		s = new Structure(nullptr, "Start_Indicator", "INDICATOR");
		starters["INDICATOR"] = s;
		sc = new StructureClass("INDICATOR", "");
		sc->setBuiltIn();
		hm_classes.push_back(sc);

		s->getProperties().add("width",120);
		s->getProperties().add("height",60);


		cell = new Widget(palette_content);
		cell->setFixedSize(Vector2i(button_width+4,35));
		b = new StructureFactoryButton(gui, "IMAGE", this, cell, 0, "IMAGE", "");
		b->setEnabled(true);
		b->setFixedSize(Vector2i(button_width, 30));
		b->setPosition(Vector2i(2,2));
		s = new Structure(nullptr, "Start_Image", "IMAGE");
		starters["IMAGE"] = s;
		sc = new StructureClass("IMAGE", "");
		sc->setBuiltIn();
		hm_classes.push_back(sc);
		s->getProperties().add("width",128);
		s->getProperties().add("height",128);

		cell = new Widget(palette_content);
		cell->setFixedSize(Vector2i(button_width+4,35));
		b = new StructureFactoryButton(gui, "RECT", this, cell, 0, "RECT", "");
		b->setEnabled(true);
		b->setFixedSize(Vector2i(button_width, 30));
		b->setPosition(Vector2i(2,2));
		s = new Structure(nullptr, "Start_Rect", "RECT");
		starters["RECT"] = s;
		sc = new StructureClass("RECT", "");
		sc->setBuiltIn();
		hm_classes.push_back(sc);
		s->getProperties().add("width",128);
		s->getProperties().add("height",128);

		cell = new Widget(palette_content);
		cell->setFixedSize(Vector2i(button_width+4,35));
		b = new StructureFactoryButton(gui, "LABEL", this, cell, 0, "LABEL", "");
		b->setEnabled(true);
		b->setFixedSize(Vector2i(button_width, 30));
		b->setPosition(Vector2i(2,2));
		s = new Structure(nullptr, "Start_Label", "LABEL");
		starters["LABEL"] = s;
		sc = new StructureClass("LABEL", "");
		sc->setBuiltIn();
		hm_classes.push_back(sc);
		s->getProperties().add("width",80);
		s->getProperties().add("height",40);

		cell = new Widget(palette_content);
		cell->setFixedSize(Vector2i(button_width+4,35));
		b = new StructureFactoryButton(gui, "TEXT", this, cell, 0, "TEXT", "");
		b->setEnabled(true);
		b->setFixedSize(Vector2i(button_width, 30));
		b->setPosition(Vector2i(2,2));
		s = new Structure(nullptr, "Start_Text", "TEXT");
		starters["TEXT"] = s;
		sc = new StructureClass("TEXT", "");
		sc->setBuiltIn();
		hm_classes.push_back(sc);
		s->getProperties().add("width",80);
		s->getProperties().add("height",40);

		cell = new Widget(palette_content);
		cell->setFixedSize(Vector2i(button_width+4,35));
		b = new StructureFactoryButton(gui, "PLOT", this, cell, 0, "PLOT", "");
		b->setEnabled(true);
		b->setFixedSize(Vector2i(button_width, 30));
		b->setPosition(Vector2i(2,2));
		s = new Structure(nullptr, "Start_Plot", "PLOT");
		starters["PLOT"] = s;
		sc = new StructureClass("PLOT", "");
		sc->setBuiltIn();
		hm_classes.push_back(sc);
		s->getProperties().add("width",256);
		s->getProperties().add("height",128);


		cell = new Widget(palette_content);
		cell->setFixedSize(Vector2i(button_width+4,35));
		b = new StructureFactoryButton(gui, "PROGRESS", this, cell, 0, "PROGRESS", "");
		b->setEnabled(true);
		b->setFixedSize(Vector2i(button_width, 30));
		b->setPosition(Vector2i(2,2));
		s = new Structure(nullptr, "Start_Progress", "PROGRESS");
		starters["PROGRESS"] = s;
		sc = new StructureClass("PROGRESS", "");
		sc->setBuiltIn();
		hm_classes.push_back(sc);
		s->getProperties().add("width",256);
		s->getProperties().add("height",32);

	}

}

Structure *StructuresWindow::createStructure(const std::string kind) {
	StructureClass *sc = findClass(kind);
	if (sc) return sc->instantiate(nullptr);
	return 0;
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

	const int tool_button_size = 32;
	Widget *items = new Widget(window);
	items->setPosition(Vector2i(1, window->theme()->mWindowHeaderHeight+1));
	items->setLayout(new BoxLayout(Orientation::Horizontal, Alignment::Fill));
	items->setFixedSize(Vector2i(window->width(), tool_button_size+5));
	items->setSize(Vector2i(window->width(), tool_button_size+5));

	ToolButton *tb = new ToolButton(items, ENTYPO_ICON_PENCIL);
	tb->setFlags(Button::ToggleButton);
	tb->setFixedSize(Vector2i(tool_button_size, tool_button_size));
	tb->setSize(Vector2i(tool_button_size, tool_button_size));

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

class ScreenSelectButton : public SelectableButton {
	public:
	ScreenSelectButton(const std::string kind, Palette *pal, nanogui::Widget *parent,
			const std::string &caption, ScreensWindow*sw)
		: SelectableButton(kind, pal, parent, caption),  screens_window(sw) {
			screen = findScreen(caption);
			if (screen)
				std::cout << "screen select button " << caption << " selects structure "
				<< screen->getName() << ":" << screen->getKind() << "\n";
			else
				std::cout << "screen select button " << caption << " has no screen\n";
		 }
	virtual void justDeselected() override {
		UserWindow *uw = screens_window->getUserWindow();
		if (uw) uw->clear();
	}
	virtual void justSelected() override {
		if (!getScreen()) setScreen(findScreen(caption()));
		UserWindow *uw = EDITOR->gui()->getUserWindow();
		if (uw && getScreen()) {
			// in edit mode the active screen is set by user actions.
			// otherwise it is only changed by a property change from the remote end
			system_settings->getProperties().add("active_screen", Value(getScreen()->getName(), Value::t_string));
			uw->setStructure(getScreen());
			uw->refresh();
		}
	}

	void setScreen( Structure *s) {
		screen = s;
	}
	Structure *getScreen() { return screen; }
	private:
	ScreensWindow *screens_window;
	Structure *screen;
};

ScreensWindow::ScreensWindow(EditorGUI *screen, nanogui::Theme *theme) : Skeleton(screen),
		Palette(PT_SINGLE_SELECT), gui(screen) {
	using namespace nanogui;
	gui = screen;
	window->setTheme(theme);
	window->setFixedSize(Vector2i(220,320));
	window->setSize(Vector2i(220,320));
	window->setPosition(Vector2i(screen->width() - 220, 40));
	window->setLayout(new BoxLayout(Orientation::Vertical,Alignment::Fill));
	window->setTitle("Screens");
	window->setVisible(false);

	const int tool_button_size = 32;
	Widget *items = new Widget(window);
	items->setPosition(Vector2i(1, window->theme()->mWindowHeaderHeight+1));
	items->setLayout(new BoxLayout(Orientation::Horizontal, Alignment::Fill));
	items->setFixedSize(Vector2i(window->width(), tool_button_size+5));
	items->setSize(Vector2i(window->width(), tool_button_size+5));

	ToolButton *tb = new ToolButton(items, ENTYPO_ICON_PLUS);
	tb->setFlags(Button::NormalButton);
	tb->setFixedSize(Vector2i(tool_button_size, tool_button_size));
	tb->setSize(Vector2i(tool_button_size, tool_button_size));
	//tb->setPosition(Vector2i(32, 64));
	UserWindow *uw = gui->getUserWindow();
	tb->setCallback([this, uw]() {
		uw->clearSelections();

		createScreenStructure();
		if (gui->getScreensWindow()) {
			this->update();
			this->selectFirst();
		}
	});

	{
		palette_scroller = new VScrollPanel(window);
		int button_width = window->width() - 20;
		palette_scroller->setFixedSize(Vector2i(window->width(), window->height() - window->theme()->mWindowHeaderHeight - tool_button_size));
		palette_scroller->setPosition( Vector2i(5, window->theme()->mWindowHeaderHeight + 10 + tool_button_size));
		palette_content = new Widget(palette_scroller);
		palette_content->setLayout(new GridLayout(Orientation::Vertical,100));
		Widget *cell = new Widget(palette_content);
		cell->setFixedSize(Vector2i(button_width+4,35));
	}
	update();
	window->performLayout(gui->nvgContext());
}

void ScreensWindow::clearSelections(Selectable * except) {
	if (except == 0)
		int x = 1;
	return Palette::clearSelections(except);
}

UserWindow *ScreensWindow::getUserWindow() {
	return gui->getUserWindow();
}

Structure *ScreensWindow::getSelectedStructure() {
	if (!hasSelections()) return 0;
	auto found = selections.begin();
	ScreenSelectButton *btn = dynamic_cast<ScreenSelectButton*>(*found);
	if (btn)
		return btn->getScreen();
	return 0;
}

void ScreensWindow::updateSelectedName() {
	if (!hasSelections()) return;
	auto found = selections.begin();
	ScreenSelectButton *btn = dynamic_cast<ScreenSelectButton*>(*found);
	if (btn) {
		Structure *s = btn->getScreen();
		btn->setCaption(s->getName());
	}
}

void ScreensWindow::update() {
	gui->getUserWindow()->clearSelections();

	if (getWindow()->visible())
		getWindow()->requestFocus();
	Selectable *current = nullptr;
	//if (hasSelections())
		//current = *(selections.begin());
	clearSelections();
	int button_width = window->width() - 20;
	int n = palette_content->childCount();
	while (n--) {
		palette_content->removeChild(0);
	}
	EditorGUI *app = gui;

	// check whether all screens are instantiated
	int created = createScreens();
	if (created)
		std::cout << "Warning: " << created << " screen instances were missing!\n";

	for (auto item : hm_structures ) {
		Structure *s = item;
		std::cout << "checking if structure " << item << " (" << s->getKind() << ") is a screen\n";
		StructureClass *sc = findClass(s->getKind());
		int count = 0;
		if (s->getKind() == "SCREEN" || (sc && sc->getBase() == "SCREEN") ) {
			++count;
			nanogui::Widget *cell = new nanogui::Widget(palette_content);
			cell->setFixedSize(Vector2i(button_width+4,35));
			ScreenSelectButton *b = new ScreenSelectButton("BUTTON", this, cell, s->getName(), this);
			b->setEnabled(true);
			b->setFixedSize(Vector2i(button_width, 30));
			b->setPosition(Vector2i(2,2));
			b->setPassThrough(true);
			b->setCallback( [app,b](){
				app->getScreensWindow()->getWindow()->requestFocus();
				app->getUserWindow()->clearSelections();
				app->getScreensWindow()->clearSelections(b);
				b->select();
			});
		}
	}

	if (!palette_content->childCount()) {
			Structure *screen = createScreenStructure();
			nanogui::Widget *cell = new nanogui::Widget(palette_content);
			cell->setFixedSize(Vector2i(button_width+4,35));
			ScreenSelectButton *b = new ScreenSelectButton("BUTTON", this, cell, screen->getName(), this);
			b->setEnabled(true);
			b->setFixedSize(Vector2i(button_width, 30));
			b->setPosition(Vector2i(2,2));
			b->setPassThrough(true);
			b->setCallback( [app,b](){
				app->getScreensWindow()->getWindow()->requestFocus();
				app->getUserWindow()->clearSelections();
				app->getScreensWindow()->clearSelections(b);
				b->select();
			});
	}
	getWindow()->performLayout(gui->nvgContext());
	if (EDITOR->isEditMode()) selectFirst();
	/*if (current) {
		SelectableButton *btn = dynamic_cast<SelectableButton*>(current);
		if (btn)
			btn->callback()();
	}*/

}

void ScreensWindow::selectFirst() {
	if (hasSelections()) return;
	int n = palette_content->childCount();
	if (!n) return;
	nanogui::Widget *cell = palette_content->childAt(0);
	if (!cell->childCount()) return;
	Selectable *s = dynamic_cast<Selectable*>(cell->childAt(0));
	if (s) s->select();
}


ViewsWindow::ViewsWindow(EditorGUI *screen, nanogui::Theme *theme) : gui(screen) {
	using namespace nanogui;
	properties = new FormHelper(screen);
	window = properties->addWindow(Eigen::Vector2i(200, 50), "Views");
	window->setTheme(theme);
	window->setVisible(false);
}

void ViewsWindow::addWindows() {
	add("Structures", gui->getStructuresWindow()->getWindow());
	add("Properties", gui->getPropertyWindow()->getWindow());
	add("Objects", gui->getObjectWindow()->getWindow());
	add("Patterns", gui->getPatternsWindow()->getWindow());
	add("Screens", gui->getScreensWindow()->getWindow());
}

void ViewsWindow::add(const std::string name, nanogui::Widget *w) {
	assert(w);
	properties->addVariable<bool> (
		name,
		  [&,name,this](bool value) mutable{
				nanogui::Widget *w = gui->getNamedWindow(name);
			  if (w) w->setVisible(value);
				gui->getViewManager().set(name, value);
			  EditorSettings::flush();
		  },
		  [&,name,this]()->bool{
			  return this->gui->getViewManager().get(name).visible;
		  });
}

void UserWindow::getPropertyNames(std::list<std::string> &names) {
	names.push_back("Screen Width");
	names.push_back("Screen Height");
	names.push_back("Screen Id");
	//names.push_back("Window Width");
	//names.push_back("Window Height");
	names.push_back("File Name");
}

void UserWindow::loadPropertyToStructureMap(std::map<std::string, std::string> &property_map) {
	property_map["Screen Width"] = "screen_width";
	property_map["Screen Height"] = "screen_height";
	property_map["Screen Id"] = "screen_id";
	property_map["File Name"] = "file_name";
}


void UserWindow::loadProperties(PropertyFormHelper *properties) {
	UserWindow *uw = this;
	{
		std::string label("Screen Title");
		properties->addVariable<std::string>(label,
				 [uw](std::string value) {
					 PanelScreen *ps = uw->getActivePanel();
					 if (ps) {
						 uw->getWindow()->setTitle(value);
						 assert(uw->structure());
						 uw->structure()->getProperties().add("Title", value);
					 }
					 uw->getWindow()->requestFocus();
				 },
				 [uw]()->std::string{
					 PanelScreen *ps = uw->getActivePanel();
					 if (ps) return uw->getWindow()->title();
					 return "";
				 });
	}
	{
	std::string label("Screen Name");
	properties->addVariable<std::string>(label,
							 [uw](std::string value) {
								 PanelScreen *ps = uw->getActivePanel();
								 if (ps) {
									 ps->setName(value);
									 assert(uw->structure());
									 uw->structure()->setName(value);
								 }
								 uw->getWindow()->requestFocus();
								 if (uw->app()->getScreensWindow()) {
									 uw->app()->getScreensWindow()->updateSelectedName();
								 }
							 },
							 [uw]()->std::string{
								 PanelScreen *ps = uw->getActivePanel();
								 if (ps) return ps->getName();
								 return "";
							 });
	}
	{
	std::string label("File Name");
	properties->addVariable<std::string>(label,
							 [uw](std::string value) {
								 std::cout << "setting file name for " << uw->structure()->getName() << " to " << value << "\n";
								 	if (uw->structure())
									uw->structure()->getInternalProperties().add("file_name", Value(value, Value::t_string));
							 },
							 [uw]()->std::string{
									if (!uw->structure()) return "";
									const Value &vx = uw->structure()->getInternalProperties().find("file_name");
									if (vx != SymbolTable::Null) return vx.asString();
									return "";
							 });
	}
	{
	std::string label("Screen Class");
	properties->addVariable<std::string>(label,
							 [uw](std::string value) {
								 assert(uw->structure());
								 StructureClass *sc = findClass(value);
								 if (sc && sc != uw->structure()->getStructureDefinition())  {
									 std::cout << "Error: structure class " << value << " already exists\n";
									 return;
								 }
								 if (uw->structure()->getStructureDefinition())
								 uw->structure()->getStructureDefinition()->setName(value);
							 },
							 [uw]()->std::string{
								 ScreensWindow *sw = (uw->app()->getScreensWindow());
								 if (!sw) return "";
								 if (!uw->structure()) return "Untitled";
								  if (!uw->structure()->getStructureDefinition()) return "Unknown";
								 return uw->structure()->getStructureDefinition()->getName();
							 });
	}
	{
	std::string label("Class File");
	properties->addVariable<std::string>(label,
							 [uw](std::string value) {
								 	if (uw->structure() && uw->structure()->getStructureDefinition())
										uw->structure()->getStructureDefinition()->getInternalProperties()
											.add("file_name", Value(value, Value::t_string));
							 },
							 [uw]()->std::string{
									if (!uw->structure() || uw->structure()->getStructureDefinition()) return "";
									const Value &vx = uw->structure()->getStructureDefinition()->getInternalProperties().find("file_name");
									if (vx != SymbolTable::Null)
										return vx.asString();
									return "";
							 });
	}
	{
	std::string label = "Window Width";
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
	label = "Screen Width";
	properties->addVariable<int>(label,
							 [uw](int value) mutable {
								 nanogui::Window *w = uw->getWindow();
								 w->setWidth(value);
								 w->setFixedWidth(value);
								 },
							 [uw]()->int{ return uw->getWindow()->width(); });
	label = "Screen Height";
	properties->addVariable<int>(label,
							 [uw](int value) mutable {
								 nanogui::Window *w = uw->getWindow();
								 w->setHeight(value);
								 w->setFixedHeight(value);
								 },
							 [uw]()->int{ return uw->getWindow()->height(); });
	label = "Screen Id";
	properties->addVariable<int>(label,
		[uw](int value) mutable {
			nanogui::Window *w = uw->getWindow();
			Structure *s = uw->structure();
			if (s) s->getProperties().add("screen_id", value);
		},
		[uw]()->int{
			Structure *s = uw->structure();
			if (s) {
				const Value &v(s->getProperties().find("screen_id"));
				long res = 0;
				if (v.asInteger(res)) return res;
			}
			return 0;
		});
	}
}

Value UserWindow::getPropertyValue(const std::string &prop) {
	if (prop == "Screen Width") return getWindow()->width();
	if (prop == "Screen Height") return getWindow()->height();
	if (prop == "Window Width") return getWindow()->width();
	if (prop == "Window Height") return getWindow()->height();
	Structure *current_screen = structure();
	if (prop == "File Name" && current_screen)
		return current_screen->getInternalProperties().find("file_name");
	return SymbolTable::Null;
}


PropertyWindow::PropertyWindow(nanogui::Screen *s, nanogui::Theme *theme) : screen(s) {
	using namespace nanogui;
	properties = new PropertyFormHelper(screen);
	//properties->setFixedSize(nanogui::Vector2i(120,28));
	//item_proxy = new ItemProxy(properties, 0);
	window = properties->addWindow(Eigen::Vector2i(30, 50), "Property List");
	window->setTheme(theme);
	window->setFixedSize(nanogui::Vector2i(260,560));

	window->setVisible(false);
}

void PropertyWindow::setVisible(bool which) { window->setVisible(which); }

void PropertyWindow::update() {
	EditorGUI *gui = dynamic_cast<EditorGUI*>(screen);
	UserWindow *uw = 0;
	if (gui) {
		uw = gui->getUserWindow();
		if (!uw) return;
		nanogui::Window *pw =getWindow();
		if (!pw) return;

		properties->clear();
		properties->setWindow(pw); // reset the grid layout
		pw->setVisible(gui->getViewManager().get("Properties").visible);
		int n = pw->children().size();

		if (uw->getSelected().size()) {
			// collect a map of all properties to their objects and then load the
			// properties that are common to all selected objects
			int num_sel = uw->getSelected().size();
			if (num_sel>1) {
				std::multimap<std::string, EditorWidget*> items;
				std::set<std::string>non_shared_properties;
				non_shared_properties.insert("Name");
				non_shared_properties.insert("Structure");
				for (auto sel : uw->getSelected()) {
					EditorWidget *ew = dynamic_cast<EditorWidget*>(sel);
					if (ew) {
						std::list<std::string> names;
						ew->getPropertyNames(names);
						for (auto pn : names) {
							if (non_shared_properties.count(pn) == 0)
								items.insert( std::make_pair(pn, ew) );
						}
					}
				}
				std::set<std::string> common;
				std::string last;
				unsigned int count = 0;
				for (auto pmap : items) {
					if (pmap.first == last) ++count;
					else {
						if (count == num_sel) common.insert(last);
						last = pmap.first;
						count = 1;
					}
				}
				if (count == num_sel) common.insert(last);
				if (common.size()) {
					std::list<EditorWidget *>widgets;
					for (auto sel : uw->getSelected()){
						EditorWidget *ew = dynamic_cast<EditorWidget*>(sel);
						assert(ew);
						widgets.push_back(ew);
					}
					for (auto prop : common) {
						std::string label(prop);
						properties->addVariable<std::string>(label,
									[widgets, label](std::string value) {
										for (auto sel : widgets) {
											assert(sel);
											sel->setProperty(label, value);
										}
									},
									[widgets, label]()->std::string{
										EditorWidget *ew = widgets.front();
										assert(ew);
										return ew->getProperty(label);
									});
						}
				}
				else {
					// dummy
					std::string label("dummy");
					int val;
					properties->addVariable<int>(label,
						[val](int value) mutable { val = value; },
						[val]()->int{ return val; });
				}

			}
			else {
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
		}
		else {
			uw->loadProperties(properties);
		}
		gui->performLayout();
	}
}

ObjectWindow::ObjectWindow(EditorGUI *screen, nanogui::Theme *theme, const char *tfn)
		: Skeleton(screen), gui(screen)
{
	using namespace nanogui;
	gui = screen;
	if (tfn && *tfn) tag_file_name = tfn;
	window->setTheme(theme);
	window->setFixedSize(Vector2i(360, 600));
	window->setSize(Vector2i(360, 600));
	window->setPosition( Vector2i(screen->width() - 360,48));
	window->setTitle("Objects");
	GridLayout *layout = new GridLayout(Orientation::Vertical,1, Alignment::Fill, 0, 3);
	window->setLayout(layout);
	items = new Widget(window);
	items->setPosition(Vector2i(1, window->theme()->mWindowHeaderHeight+1));
	items->setLayout(new BoxLayout(Orientation::Vertical));
	items->setFixedSize(Vector2i(window->width(), window->height() - window->theme()->mWindowHeaderHeight));

	search_box = new nanogui::TextBox(items);
	search_box->setFixedSize(Vector2i(200, 25));
	search_box->setPosition( nanogui::Vector2i(5, 5));
	search_box->setValue("");
	search_box->setEnabled(true);
	search_box->setEditable(true);
	search_box->setAlignment(TextBox::Alignment::Left);
	search_box->setCallback([&,this](const std::string &filter)->bool {
		std::string group = tab_region->tabLabelAt(tab_region->activeTab());
		loadItems(group, search_box->value());
		return true;
	});
	tab_region = items->add<TabWidget>();
	nanogui::Widget *layer = tab_region->createTab("Local");
	layer->setLayout(new GroupLayout());
	layers["Local"] = layer;
	current_layer = layer;
	tab_region->setActiveTab(0);

	if (tag_file_name.length()) loadTagFile(tag_file_name);
	window->performLayout(screen->nvgContext());
	window->setVisible(false);
}

nanogui::Widget *ObjectWindow::createTab(const std::string tags) {
	using namespace nanogui;
	const int search_height = 36;
	Widget *container = nullptr;
	std::string tab_name = shortName(tags);
	if (tags.length()) {
		auto found = layers.find(tab_name);
		if (found != layers.end())
			nanogui::Widget *container = (*found).second;
		else {
			container = tab_region->createTab(shortName(tab_name));
			container->setLayout(new GroupLayout());
			layers[tab_name] = container;
		}
		current_layer = container;
		VScrollPanel *palette_scroller = new VScrollPanel(container);
		palette_scroller->setPosition( Vector2i(1,search_height+5));
		palette_scroller->setFixedSize(Vector2i(window->width() - 10, window->height() - window->theme()->mWindowHeaderHeight - search_height));
		palette_content = new Widget(palette_scroller);
		palette_content->setFixedSize(Vector2i(window->width() - 15, window->height() - window->theme()->mWindowHeaderHeight-5 - search_height));
		return palette_content;
	}
	else
		return nullptr;
}

void ObjectWindow::loadTagFile(const std::string tags) {
	using namespace nanogui;
	const int search_height = 36;
	nanogui::Widget *palette_content = createTab(tags);
	if (!palette_content) return;
	tag_file_name = tags;

	createPanelPage(tag_file_name.c_str(), palette_content);
	GridLayout *palette_layout = new GridLayout(Orientation::Horizontal,1,Alignment::Fill);
	palette_layout->setSpacing(1);
	palette_layout->setMargin(4);
	palette_layout->setColAlignment(nanogui::Alignment::Fill);
	palette_layout->setRowAlignment(nanogui::Alignment::Fill);
	palette_content->setLayout(palette_layout);

	if (palette_content->childCount() == 0) {
		Widget *cell = new Widget(palette_content);
		cell->setFixedSize(Vector2i(window->width()-32,35));
		new Label(cell, "No match");
	}

	window->performLayout(gui->nvgContext());
}

ThemeWindow::ThemeWindow(EditorGUI *screen, nanogui::Theme *theme) :gui(screen) {
	using namespace nanogui;
	properties = new PropertyFormHelper(screen);
	window = properties->addWindow(Eigen::Vector2i(80, 50), "Theme Properties");
	window->setTheme(theme);
	loadTheme(theme);
	window->setVisible(false);
}

void ThemeWindow::loadTheme(nanogui::Theme *theme) {
	nanogui::Window *pw =getWindow();
	if (!pw) return;
	EditorGUI *app = gui;

	properties->clear();
	properties->setWindow(pw); // reset the grid layout
	properties->addVariable("Standard Font Size", theme->mStandardFontSize);
	properties->addVariable("Button Font Size", theme->mButtonFontSize);
	//properties->addVariable("TextBox Font Size", theme->mTextBoxFontSize);
	properties->addVariable<int> ("TextBox Font Size",
									  [theme, app](int value) {
										  theme->mTextBoxFontSize = value;
										  app->getUserWindow()->getWindow()->performLayout( app->nvgContext() );
									   },
									  [theme]()->int{ return theme->mTextBoxFontSize; });
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
	gui->performLayout();
}

void setupTheme(nanogui::Theme *theme) {
	using namespace nanogui;
	theme->mStandardFontSize                 = 20;
	theme->mButtonFontSize                   = 20;
	theme->mTextBoxFontSize                  = -1;
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
	theme->mButtonGradientTopPushed          = Color(180, 255);
	theme->mButtonGradientBotPushed          = Color(196, 255);

	/* Window-related */
	theme->mWindowFillUnfocused              = Color(220, 230);
	theme->mWindowFillFocused                = Color(225, 230);
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

	UserWindowWin *uww = new UserWindowWin(this, "Untitled");
	user_screens.push_back(uww);
	nanogui::Theme *uwTheme = new nanogui::Theme(nvgContext());
	setupTheme(uwTheme);
	uwTheme->mWindowHeaderHeight = 0;
	w_user = new UserWindow(this, uwTheme, uww);
	w_theme = new ThemeWindow(this, theme);
	w_properties = new PropertyWindow(this, theme);
	w_toolbar = new Toolbar(this, theme);
	w_startup = new StartupWindow(this, theme);
	w_objects = new ObjectWindow(this, theme);

	w_structures = StructuresWindow::create(this, theme);

	w_patterns = new PatternsWindow(this, theme);
	w_screens = new ScreensWindow(this, theme);
	w_views = new ViewsWindow(this, theme);
/*
	ConfirmDialog *cd = new ConfirmDialog(this, "Humid V0.23");
	cd->setCallback([cd,this]{
		cd->setVisible(false);
		this->setState(EditorGUI::GUISELECTPROJECT);
	});
	window = cd->getWindow();
	window->setTheme(theme);
	*/

	EditorSettings::applySettings("MainWindow", this);
	EditorSettings::applySettings("ThemeSettings", w_theme->getWindow());
	EditorSettings::applySettings("Properties", w_properties->getWindow());
	EditorSettings::applySettings("Structures", w_structures->getWindow());
	EditorSettings::applySettings("Patterns", w_patterns->getWindow());
	EditorSettings::applySettings("Objects", w_objects->getWindow());
	EditorSettings::applySettings("ScreensWindow", w_screens->getWindow());

	// delayed adding windows to the view manager window until the visibility settings are loaded.
	w_views->addWindows();

	EditorSettings::add("MainWindow", this);
	EditorSettings::add("ThemeSettings", w_theme->getWindow());
	EditorSettings::add("Properties", w_properties->getWindow());
	EditorSettings::add("Structures", w_structures->getWindow());
	EditorSettings::add("Patterns", w_patterns->getWindow());
	EditorSettings::add("Objects", w_objects->getWindow());
	EditorSettings::add("ScreensWindow", w_screens->getWindow());

	uww->setMoveListener([](nanogui::Window *value) {
		updateSettingsStructure("MainWindow", value);
	});
	w_objects->getSkeletonWindow()->setMoveListener(
		[](nanogui::Window *value) { updateSettingsStructure("Objects", value); }
	);
	w_patterns->getSkeletonWindow()->setMoveListener(
		[](nanogui::Window *value) { updateSettingsStructure("Patterns", value); }
	);
	w_structures->getSkeletonWindow()->setMoveListener(
		[](nanogui::Window *value) { updateSettingsStructure("Structures", value); }
	);
	w_screens->getSkeletonWindow()->setMoveListener(
		[](nanogui::Window *value) { updateSettingsStructure("ScreensWindow", value); }
	);
	w_screens->update();
	performLayout(mNVGContext);
	setState(EditorGUI::GUISELECTPROJECT);
}


void EditorGUI::setState(EditorGUI::GuiState s) {
	bool editmode = false;
	bool done = false;
	while (!done) {
		switch(s) {
			case GUIWELCOME:
				done = true;
				break;
			case GUISELECTPROJECT:
				if (hm_structures.size()>1) { // files were specified on the commandline
					getStartupWindow()->setVisible(false);
					getScreensWindow()->update();
					
					Structure *settings = EditorSettings::find("EditorSettings");
					assert(settings);
					const Value &project_base_v(settings->getProperties().find("project_base"));
					if (project_base_v == SymbolTable::Null) {
						s = GUICREATEPROJECT; done = false; break;
					}
					s = GUIWORKING;
					done = false;
				}
				else {
					window = getStartupWindow()->getWindow();
					window->setVisible(true);
					done = true;
				}
				break;
			case GUICREATEPROJECT:
			{
				getStartupWindow()->setVisible(false);
				Structure *settings(getSettings());
				const Value &path(settings->getProperties().find("project_base"));
				if (path != SymbolTable::Null)
					project = new EditorProject(path.asString().c_str());
				if (!project)
					project = new EditorProject("UntitledProject");
				getUserWindow()->setStructure(createScreenStructure());
				getUserWindow()->setVisible(true);
				getToolbar()->setVisible(!run_only);
				done = true;
			}
				break;
			case GUIEDITMODE:
				editmode = true;
				if (false){
					Value remote_screen(system_settings->getProperties().find("remote_screen"));
					if (remote_screen != SymbolTable::Null) {
						LinkableProperty *lp = findLinkableProperty(remote_screen.asString());
						if (lp)
							lp->apply();
							if (w_screens && !w_screens->hasSelections()) w_screens->selectFirst();
					}
				}
				// fall through
			case GUIWORKING:
				getUserWindow()->setVisible(true);
				//getScreensWindow()->selectFirst();
				getToolbar()->setVisible(!run_only);
				if (getPropertyWindow()) {
					nanogui::Window *w = getPropertyWindow()->getWindow();
					w->setVisible(editmode && views.get("Properties").visible);
				}
				if (getPatternsWindow()) {
					nanogui::Window *w = getPatternsWindow()->getWindow();
					w->setVisible(editmode && views.get("Patterns").visible);
				}
				if (getStructuresWindow()) {
					nanogui::Window *w = getStructuresWindow()->getWindow();
					w->setVisible(editmode && views.get("Structures").visible);
				}
				if (getObjectWindow()) {
					nanogui::Window *w = getObjectWindow()->getWindow();
					w->setVisible(editmode && views.get("Objects").visible);
				}
				if (getScreensWindow()) {
					nanogui::Window *w = getScreensWindow()->getWindow();
					w->setVisible(editmode && views.get("ScreensWindow").visible);
				}
				Value remote_screen(system_settings->getProperties().find("remote_screen"));
				if (remote_screen != SymbolTable::Null) {
					LinkableProperty *lp = findLinkableProperty(remote_screen.asString());
					if (lp) {
						lp->apply();
						std::cout <<"\nWorking mode: setting screen to " << lp->value() << "\n\n";
						//if (lp->value() != SymbolTable::Null)
						//	startup = sRELOAD;
					}
				}

				done = true;
				break;
		}
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

	assert(w_user->structure());
	StructureClass *screen_sc = w_user->structure()->getStructureDefinition();
	assert(screen_sc);

	unsigned int offset = 0;
	for(Selectable *sel : selections) {
		SelectableButton *item = dynamic_cast<SelectableButton*>(sel);
		if (!item) continue;
		std::cout << "creating instance of " << item->getClass() << "\n";
		nanogui::Widget *w = item->create(window);
		if (w) {
			EditorWidget *ew = dynamic_cast<EditorWidget*>(w);
			Parameter param(ew->getName());
			ew->updateStructure();
			param.machine = ew->getDefinition();
			screen_sc->addLocal(param);
			ew->getDefinition()->setOwner(w_user->structure());

			//if (ew) ew->setName( NamedObject::nextName(ew) );
			Vector2i pos(p - window->position() - w->size()/2 + nanogui::Vector2i(0, offset));
			w->setPosition(fixPlacement(w, window, pos));
			offset += w->height() + 8;
		}
	}
	window->performLayout(nvgContext());
	for (auto child : window->children()) {
		EditorWidget *ew = dynamic_cast<EditorWidget*>(child);
		if (ew) ew->updateStructure();
	}

	window->addChild(drag_handle);
	drag_handle->setPropertyMonitor(pm);
	drag_handle->decRef();
}

bool EditorGUI::mouseButtonEvent(const nanogui::Vector2i &p, int button, bool down, int modifiers) {

	using namespace nanogui;

	nanogui::Window *window = w_user->getWindow();
	if (!window || !window->visible()) return Screen::mouseButtonEvent(p, button, down, modifiers);

	Widget *clicked = findWidget(p);

	Widget *ww = dynamic_cast<Widget*>(window);

	bool is_user = EDITOR->gui()->getUserWindow()->getWindow()->focused();

	if (button != GLFW_MOUSE_BUTTON_1 || !is_user || !window->contains(p /*- window->position()*/)) {
		if (!clicked) return Screen::mouseButtonEvent(p, button, down, modifiers);
		Widget *parent = clicked->parent();
		while (parent && parent->parent()) { clicked = parent; parent = clicked->parent(); }
		if (!clicked->focused() && parent != window) clicked->requestFocus();
		return Screen::mouseButtonEvent(p, button, down, modifiers);
	}

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

bool isURL(const std::string name) {
	//if (matches(name.c_str(), "((http[s]?|ftp):/)?/?([^:/\\s]+)((/\\w+)*/)([\\w-\\.]+[^#?\\s]+)(.*)?(#[\\w\\-]+)?")) return true;
	return matches(name.c_str(), "^http://.*");
}

GLuint EditorGUI::getImageId(const char *source, bool reload) {
	GLuint blank_id = 0;
	std::string blank_name = "images/blank";
	std::string name(source);
	for(auto it = mImagesData.begin(); it != mImagesData.end(); ++it) {
		const GLTexture &tex = (*it).first;
		if (tex.textureName() == name) {
			cout << "found image " << tex.textureName() << "\n";
			return tex.texture();
		}
		else if (blank_name == tex.textureName())
			blank_id = tex.texture();
	}
	if (isURL(name)) {
		std::string cache_name = "cache";
		if (!boost::filesystem::exists(cache_name))
			boost::filesystem::create_directory(cache_name);
		cache_name += "/" + shortName(name) + "." + extn(name);
		// example: http://www.valeparksoftwaredevelopment.com/cmsimages/logo-full-1.png
		if (!boost::filesystem::exists(cache_name) && !get_file(name, cache_name) ) {
			std::cerr << "Error fetching image file\n";
			return blank_id;
		}
		name = cache_name;
	}
	std::string tex_name = shortName(name);
	auto found = texture_cache.find(tex_name);
	if (!reload && found != texture_cache.end())
		return (*found).second->texture.texture();
	else if (reload || found == texture_cache.end()) {
		if (found != texture_cache.end()) {
			Texture *old = (*found).second;
			texture_cache.erase(found);
			delete old;
		}

		// not already loaded, attempt to load from file
		GLTexture tex(tex_name);
		try {
			auto tex_data = tex.load(name);
			GLuint res = tex.texture();
			texture_cache[tex_name] = new Texture( std::move(tex), std::move(tex_data));
			std::cout << "added texture " << tex_name << " id " << res << " to cache\n";
			return res;
		}
		catch(std::invalid_argument &err) {
			std::cerr << err.what() << "\n";
		}
	}
	return blank_id;
}
nanogui::Vector2i fixPositionInWindow(const nanogui::Vector2i &pos, const nanogui::Vector2i &siz, const nanogui::Vector2i &area);

bool EditorGUI::resizeEvent(const Vector2i &new_size) {
	if (old_size == new_size) {
		return false;
	}

/*
	int width = 1024, height = 768;
	if (glfwWindow()) {
		glfwGetFramebufferSize(glfwWindow(), &width, &height);
		glViewport(0, 0, width, height);
		//glViewport(0, 0, 2880, 1800);
	}
	*/

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

		if (pos.x() < 0) pos.x() = 0;
		if (pos.y() < 0) pos.y() = 0;
		if (pos.x() + w->width() > new_size.x()) pos.x() = new_size.x() - w->width();
		if (pos.y() + w->height() > new_size.y()) pos.y() = new_size.y() - w->height();

		item.first->setPosition(pos);
	}
	//cout << "\n";
	old_size = mSize;
	if (w_user) {
		w_user->getWindow()->setFixedSize(new_size);
		w_user->getWindow()->setPosition(nanogui::Vector2i(0,0));
		performLayout();
	}
	return true;
}

LinkableProperty *EditorGUI::findLinkableProperty(const std::string name) {
	std::lock_guard<std::recursive_mutex>  lock(linkables_mutex);
	std::map<std::string, LinkableProperty*>::iterator found = linkables.find(name);
	if (found == linkables.end()) return 0;
	return (*found).second;
}

void EditorGUI::handleClockworkMessage(ClockworkClient::Connection *conn, unsigned long now, const std::string &op, std::list<Value> *message) {
	if (op == "UPDATE") {
		if (!this->getUserWindow()) return;

		int pos = 0;
		std::string name;
		long val = 0;
		double dval = 0.0;
		CircularBuffer *buf = 0;
		LinkableProperty *lp = 0;
		for (auto &v: *message) {
			if (pos == 2) {
				name = v.asString();
				lp = findLinkableProperty(name);
				buf = w_user->getValues(name);
				if (collect_history) {
					if (!buf && lp && lp->dataType() != CircularBuffer::STR) {
						buf = w_user->addDataBuffer(name, lp->dataType(), collect_history);
						if (!buf) std::cout << "no buffer for " << name << "\n";
					}
				}
			}
			else if (pos == 4) {
				if (lp)
					lp->setValue(v);
				if (buf) {
					CircularBuffer::DataType dt = buf->getDataType();
					if (v.asInteger(val)) {
						if (dt == CircularBuffer::INT16) {
							buf->addSample(now, (int16_t)(val & 0xffff));
							//std::cout << "adding sample: " << name << " t: " << now << " " << (int16_t)(val & 0xffff) << " count: " << buf->length() << "\n";
						}
						else if (dt == CircularBuffer::INT32) {
							buf->addSample(now, (int32_t)(val & 0xffffffff));
							//std::cout << "adding sample: " << name << " t: " << now << " " << (int32_t)(val & 0xffffffff) << " count: " << buf->length() << "\n";
						}
						else
							buf->addSample(now, val);
					}
					else if (v.asFloat(dval)) {
						buf->addSample(now, dval);
					}
					//else
					//	std::cout << "cannot interpret " << name << " value '" << v << "' as an integer or float\n";
				}
			}
			++pos;
		}
	}
	//else
	//	std::cout << op << "\n";
}

void EditorGUI::processModbusInitialisation(const std::string group_name, cJSON *obj) {
	int num_params = cJSON_GetArraySize(obj);
	if (num_params)
	{
		for (int i=0; i<num_params; ++i)
		{
			cJSON *item = cJSON_GetArrayItem(obj, i);
			if (item->type == cJSON_Array)
			{
				Value group(MessageEncoding::valueFromJSONObject(cJSON_GetArrayItem(item, 0), 0));
				Value addr(MessageEncoding::valueFromJSONObject(cJSON_GetArrayItem(item, 1), 0));
				Value kind(MessageEncoding::valueFromJSONObject(cJSON_GetArrayItem(item, 2), 0));
				Value name(MessageEncoding::valueFromJSONObject(cJSON_GetArrayItem(item, 3), 0));
				Value len(MessageEncoding::valueFromJSONObject(cJSON_GetArrayItem(item, 4), 0));
				Value value(MessageEncoding::valueFromJSONObject(cJSON_GetArrayItem(item, 5), 0));
				if (DEBUG_BASIC)
					std::cout << name << ": " << group << " " << addr << " " << len << " " << value <<  "\n";
				if (value.kind == Value::t_string) {
					std::string valstr = value.asString();
					//insert((int)group.iValue, (int)addr.iValue-1, valstr.c_str(), valstr.length()+1); // note copying null
				}

				if (name.kind == Value::t_string || name.kind == Value::t_symbol) {

					size_t n = name.sValue.length();
					const char *p = name.sValue.c_str();
					if (n>2 && p[0] == '"' && p[n-1] == '"')
					{
						std::cout << "removing quotes from " << name << "\n";
						char buf[n];
						memcpy(buf, p+1, n-2);
						buf[n-2] = 0;
						name = Value(buf, Value::t_string);
					}
				}

				std::string prop_name(name.asString());
				{
					size_t p = prop_name.find(".cmd_");

					if (p != std::string::npos)
						prop_name = prop_name.erase(p+1,4);
				}


				//else
				//	insert((int)group.iValue, (int)addr.iValue-1, (int)value.iValue, len.iValue);
				LinkableProperty *lp = findLinkableProperty(prop_name);
				if (!lp) {
					std::lock_guard<std::recursive_mutex>  lock(linkables_mutex);
					char buf[10];
					snprintf(buf, 10, "'%d%4d", (int)group.iValue, (int)addr.iValue);
					std::string addr_str(buf);

					lp = new LinkableProperty(group_name, group.iValue, prop_name, addr_str, "", len.iValue);
					linkables[prop_name] = lp;
				}
				if (lp) {
					if (group.iValue != lp->address_group())
						std::cout << prop_name << " change of group from " << lp->address_group() << " to " << group << "\n";
					if (addr.iValue != lp->address())
						std::cout << prop_name << " change of address from " << lp->address() << " to " << addr << "\n";
					lp->setAddressStr(group.iValue, addr.iValue);
					lp->setValue(value);

					//w_user->fixLinks(lp);

					if (collect_history) {
						CircularBuffer *buf = getUserWindow()->getValues(prop_name);
						if (buf) {
							long v;
							double fv;
							buf->clear();
							if (value.asInteger(v))
								buf->addSample( buf->getZeroTime(), v);
							else if (value.asFloat(fv))
								buf->addSample( buf->getZeroTime(), fv);
						}
					}

				}
			}
			else
			{
				char *node = cJSON_Print(item);
				std::cerr << "item " << i << " is not of the expected format: " << node << "\n";
				free(node);
			}
		}
	}
	std::cout << "Total linkable properties is now: " << linkables.size() << "\n";
}

void EditorGUI::update(ClockworkClient::Connection *connection) {
	if (connection->getStartupState() != sDONE && connection->getStartupState() != sRELOAD) {
		// if the tag file is loaded, get initial values
		if (/*linkables.size() && */ connection->getStartupState() == sINIT && connection) {
			std::cout << "Sending data initialisation request\n";
			connection->setState(sSENT);
			queueMessage( connection->getName(), "MODBUS REFRESH",
				[this, connection](std::string s) {
					if (s != "failed") {
						cJSON *obj = cJSON_Parse(s.c_str());
						if (!obj) {
							connection->setState(sINIT);
							return;
						}
						if (obj->type == cJSON_Array) {
							processModbusInitialisation(connection->getName(), obj);
							w_objects->rebuildWindow();
							w_user->setStructure(w_user->structure());
							const Value remote_screen(system_settings->getProperties().find("remote_screen"));
							if (remote_screen != SymbolTable::Null) {
								LinkableProperty *lp = findLinkableProperty(remote_screen.asString());
								if (lp) {
									lp->link(getUserWindow());
								}
							}

						}
						connection->setState(sRELOAD);
					}
					else
						connection->setState(sINIT);
				}
			);
		}
	}

	if (connection->getStartupState() == sDONE || connection->getStartupState() == sRELOAD) {
		if (w_user) {
			//bool changed = false;
			const Value &active(system_settings->getProperties().find("active_screen"));
			if (active != SymbolTable::Null) {
				if (connection->getStartupState() == sRELOAD || (w_user->structure() && w_user->structure()->getName() != active.asString())) {
					Structure *s = findScreen(active.asString());
					if (connection->getStartupState() == sRELOAD || (s && w_user->structure() != s) ) {
						w_user->getWindow()->requestFocus();
						w_user->clearSelections();
						w_user->setStructure(s);
						std::cout << "Loaded active screen " << active << "\n";
						//changed = true;
						if (connection->getStartupState() == sRELOAD) connection->setState(sDONE);
					}
					else if (connection->getStartupState() != sRELOAD) std::cout << "Active screen " << active << " cannot be found\n";
				}
			}
			else std::cout << "No active screen has been selected\n";
			//if (changed)
			//	w_user->update();
		}
	}

	if (w_user)
		w_user->update();
	if (needs_update) {
		w_properties->getWindow()->performLayout(nvgContext());
		needs_update = false;
	}
	EditorSettings::flush();
}

bool EditorGUI::keyboardEvent(int key, int scancode , int action, int modifiers) {
	if (EDITOR->isEditMode()) {
		return nanogui::Screen::keyboardEvent(key, scancode, action, modifiers);
	}
	else {
		if (key == GLFW_KEY_ESCAPE) {
			w_user->getWindow()->requestFocus();
			return true;
		}
		if (action == GLFW_PRESS ) {
			bool function_key = false;
			std::string key_name = "";
			if ((key >= GLFW_KEY_0 && key <= GLFW_KEY_9)) {
				char buf[10];
				snprintf(buf, 10, "KEY_%d", key - GLFW_KEY_0);
				key_name = buf;
			}
			else if ((key >= GLFW_KEY_F1 && key <= GLFW_KEY_F25)) {
				char buf[10];
				snprintf(buf, 10, "KEY_F%d", key - GLFW_KEY_F1+1);
				key_name = buf;
				function_key = true;
			}
			else if ((key >= GLFW_KEY_A && key <= GLFW_KEY_Z)) {
				char buf[10];
				snprintf(buf, 10, "KEY_%c", key - GLFW_KEY_A + 'A');
				key_name = buf;
			}
			if (!function_key && nanogui::Screen::keyboardEvent(key, scancode, action, modifiers)) return true;

			if (key_name.length()) {
				std::cout << "detected key: " << key_name << "\n";
				std::string conn;
				StructureClass *sc = w_user->structure()->getStructureDefinition();
				if (sc) {
					std::map<std::string, Value>::iterator found = sc->getOptions().find(key_name);
					if (found != sc->getOptions().end())
						conn = (*found).second.asString();
				}
				if (conn.empty() && system_settings) {
					sc = system_settings->getStructureDefinition();
					if (!sc) {
						std::cout << "no class\n";
						return false;
					}
					std::map<std::string, Value>::iterator found = sc->getOptions().find(key_name);
					if (found != sc->getOptions().end())
						conn = (*found).second.asString();
				}
				if (conn.length()) {				
					std::cout << "found key mapping: " << conn << "\n";
					size_t dpos = conn.find(':');
					if (dpos != std::string::npos) {
						std::string remote = conn.substr(dpos+1);
						conn = conn.erase(dpos);

						LinkableProperty *lp = findLinkableProperty(remote);
						if (lp) {
							std::string msgon = getIODSyncCommand(conn, 0, lp->address(), 1);
							queueMessage(conn, msgon, [](std::string s){std::cout << ": " << s << "\n"; });
							std::string msgoff = getIODSyncCommand(conn, 0, lp->address(), 0);
							queueMessage(conn, msgoff, [](std::string s){std::cout << ": " << s << "\n"; });
							return true;
						}
					}
					else {
						//search the current screen for a button with this name and click it
						for (auto w : w_user->getWindow()->children()) {
							EditorWidget *ew = dynamic_cast<EditorWidget*>(w);
							EditorTextBox *et = dynamic_cast<EditorTextBox*>(w);
							EditorButton *eb = dynamic_cast<EditorButton*>(w);
							if (et && et->getName() == conn) {
								w->requestFocus();
								et->selectAll();
								return true;
							}
							else if (eb && eb->getName() == conn) {
								if (eb->callback()) eb->callback()();
								else if (eb->changeCallback()) {
									eb->changeCallback()(!eb->pushed());
									return true;
								}
							}
						}
					}
				}
			}
		}
	}
	return nanogui::Screen::keyboardEvent(key, scancode, action, modifiers);
}


CircularBuffer *UserWindow::getDataBuffer(const std::string item) {
	std::map<std::string, CircularBuffer *>::iterator found = data.find(item);
	if (found == data.end()) return nullptr;
	//	addDataBuffer(item, gui->sampleBufferSize());
	return (*found).second;
}

void UserWindow::refresh() { window->performLayout(gui->nvgContext()); }

nanogui::Window *ObjectWindow::createPanelPage(
		   const char *filename,
		   nanogui::Widget *palette_content) {
	using namespace nanogui;

	if (filename && palette_content) {
		std::string modbus_settings(filename);
		std::ifstream init(modbus_settings);

		if (init.good()) {
			importModbusInterface(filename, init, palette_content, window);
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

void EditorWidget::loadProperties(PropertyFormHelper* properties) {
	nanogui::Widget *w = dynamic_cast<nanogui::Widget*>(this);
	if (w) {
		properties->addVariable<std::string> ("Structure",
									  [&,w](const std::string value) { },
									  [&,w]()->std::string{ return getDefinition()->getKind(); });
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
											  [&](std::string value) mutable{
													setName(value);
													if (getDefinition()) {
														getDefinition()->setName(value);
													}
												},
											  [&]()->std::string{ return getName(); });
		properties->addVariable<int> ("Font Size",
									  [&,w](int value) mutable{ w->setFontSize(value); },
									  [&,w]()->int{ return w->fontSize(); });
		properties->addVariable<int> ("Tab Position",
									  [&,w](int value) mutable{ setTabPosition(value); },
									  [&,w]()->int{ return tabPosition(); });
		properties->addVariable<int> ("Value Scale",
									  [&,w](int value) mutable{ setValueScale(value); },
									  [&,w]()->int{ return valueScale(); });
		properties->addVariable<std::string> ("Patterns",
											  [&,w](std::string value) mutable{ setPatterns(value); },
											  [&,w]()->std::string{ return patterns(); }
											  );
		EditorGUI *gui = EDITOR->gui();
		properties->addButton("Link to Remote", [&,gui,this,properties]() mutable{
										if (gui->getObjectWindow()->hasSelections() && gui->getObjectWindow()->hasSelections()) {
											gui->getUserWindow()->getWindow()->requestFocus();
											std::string items;
											for (auto sel : gui->getObjectWindow()->getSelected()) {
												ObjectFactoryButton *btn = dynamic_cast<ObjectFactoryButton*>(sel);
												LinkableProperty *lp = gui->findLinkableProperty(btn->tagName());
												if (remote) remote->unlink(this);
												remote = lp;
												setProperty("Remote", btn->tagName());
												break;
											}
											gui->getObjectWindow()->clearSelections();
											//properties->refresh();
											gui->getUserWindow()->clearSelections();
											gui->getPropertyWindow()->update();
											//gui->getUserWindow()->select(this);
										}
									});

	}
}

void EditorTextBox::draw(NVGcontext* ctx) {
	using namespace nanogui;

    Widget::draw(ctx);

    NVGpaint bg = nvgBoxGradient(ctx,
        mPos.x() + 1, mPos.y() + 1 + 1.0f, mSize.x() - 2, mSize.y() - 2,
        3, 4, Color(255, 32), Color(32, 32));
    NVGpaint fg1 = nvgBoxGradient(ctx,
        mPos.x() + 1, mPos.y() + 1 + 1.0f, mSize.x() - 2, mSize.y() - 2,
        3, 4, Color(150, 32), Color(32, 32));
    NVGpaint fg2 = nvgBoxGradient(ctx,
        mPos.x() + 1, mPos.y() + 1 + 1.0f, mSize.x() - 2, mSize.y() - 2,
        3, 4, nvgRGBA(255, 0, 0, 100), nvgRGBA(255, 0, 0, 50));

    nvgBeginPath(ctx);
    nvgRoundedRect(ctx, mPos.x() + 1, mPos.y() + 1 + 1.0f, mSize.x() - 2,
                   mSize.y() - 2, 3);

    if (mEditable && focused())
        mValidFormat ? nvgFillPaint(ctx, fg1) : nvgFillPaint(ctx, fg2);
    else if (mSpinnable && mMouseDownPos.x() != -1)
        nvgFillPaint(ctx, fg1);
    else
        nvgFillPaint(ctx, bg);

    nvgFill(ctx);

    nvgBeginPath(ctx);
    nvgRoundedRect(ctx, mPos.x() + 0.5f, mPos.y() + 0.5f, mSize.x() - 1,
                   mSize.y() - 1, 2.5f);
    nvgStrokeColor(ctx, Color(0, 48));
    nvgStroke(ctx);

    nvgFontSize(ctx, fontSize());
    nvgFontFace(ctx, "sans");
    Vector2i drawPos(mPos.x(), mPos.y() + mSize.y() * 0.5f + 1);

    float xSpacing = mSize.y() * 0.3f;

    float unitWidth = 0;

    if (mUnitsImage > 0) {
        int w, h;
        nvgImageSize(ctx, mUnitsImage, &w, &h);
        float unitHeight = mSize.y() * 0.4f;
        unitWidth = w * unitHeight / h;
        NVGpaint imgPaint = nvgImagePattern(
            ctx, mPos.x() + mSize.x() - xSpacing - unitWidth,
            drawPos.y() - unitHeight * 0.5f, unitWidth, unitHeight, 0,
            mUnitsImage, mEnabled ? 0.7f : 0.35f);
        nvgBeginPath(ctx);
        nvgRect(ctx, mPos.x() + mSize.x() - xSpacing - unitWidth,
                drawPos.y() - unitHeight * 0.5f, unitWidth, unitHeight);
        nvgFillPaint(ctx, imgPaint);
        nvgFill(ctx);
        unitWidth += 2;
    } else if (!mUnits.empty()) {
        unitWidth = nvgTextBounds(ctx, 0, 0, mUnits.c_str(), nullptr, nullptr);
        nvgFillColor(ctx, Color(255, mEnabled ? 64 : 32));
        nvgTextAlign(ctx, NVG_ALIGN_RIGHT | NVG_ALIGN_MIDDLE);
        nvgText(ctx, mPos.x() + mSize.x() - xSpacing, drawPos.y(),
                mUnits.c_str(), nullptr);
        unitWidth += 2;
    }

    float spinArrowsWidth = 0.f;

    if (mSpinnable && !focused()) {
        spinArrowsWidth = 14.f;

        nvgFontFace(ctx, "icons");
        nvgFontSize(ctx, ((mFontSize < 0) ? mTheme->mButtonFontSize : mFontSize) * 1.2f);

        bool spinning = mMouseDownPos.x() != -1;

        /* up button */ {
            bool hover = mMouseFocus && spinArea(mMousePos) == SpinArea::Top;
            nvgFillColor(ctx, (mEnabled && (hover || spinning)) ? mTheme->mTextColor : mTheme->mDisabledTextColor);
            auto icon = utf8(ENTYPO_ICON_CHEVRON_UP);
            nvgTextAlign(ctx, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
            Vector2f iconPos(mPos.x() + 4.f,
                             mPos.y() + mSize.y()/2.f - xSpacing/2.f);
            nvgText(ctx, iconPos.x(), iconPos.y(), icon.data(), nullptr);
        }

        /* down button */ {
            bool hover = mMouseFocus && spinArea(mMousePos) == SpinArea::Bottom;
            nvgFillColor(ctx, (mEnabled && (hover || spinning)) ? mTheme->mTextColor : mTheme->mDisabledTextColor);
            auto icon = utf8(ENTYPO_ICON_CHEVRON_DOWN);
            nvgTextAlign(ctx, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
            Vector2f iconPos(mPos.x() + 4.f,
                             mPos.y() + mSize.y()/2.f + xSpacing/2.f + 1.5f);
            nvgText(ctx, iconPos.x(), iconPos.y(), icon.data(), nullptr);
        }

        nvgFontSize(ctx, fontSize());
        nvgFontFace(ctx, "sans");
    }

    switch (mAlignment) {
        case Alignment::Left:
            nvgTextAlign(ctx, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
            drawPos.x() += xSpacing + spinArrowsWidth;
            break;
        case Alignment::Right:
            nvgTextAlign(ctx, NVG_ALIGN_RIGHT | NVG_ALIGN_MIDDLE);
            drawPos.x() += mSize.x() - unitWidth - xSpacing;
            break;
        case Alignment::Center:
            nvgTextAlign(ctx, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
            drawPos.x() += mSize.x() * 0.5f;
            break;
    }

    nvgFontSize(ctx, fontSize());
    nvgFillColor(ctx,
                 mEnabled ? mTheme->mTextColor : mTheme->mDisabledTextColor);

    // clip visible text area
    float clipX = mPos.x() + xSpacing + spinArrowsWidth - 1.0f;
    float clipY = mPos.y() + 1.0f;
    float clipWidth = mSize.x() - unitWidth - spinArrowsWidth - 2 * xSpacing + 2.0f;
    float clipHeight = mSize.y() - 3.0f;

    nvgSave(ctx);
    nvgIntersectScissor(ctx, clipX, clipY, clipWidth, clipHeight);

    Vector2i oldDrawPos(drawPos);
    drawPos.x() += mTextOffset;

    if (mCommitted) {
        nvgText(ctx, drawPos.x(), drawPos.y(), mValue.c_str(), nullptr);
    } else {
        const int maxGlyphs = 1024;
        NVGglyphPosition glyphs[maxGlyphs];
        float textBound[4];
        nvgTextBounds(ctx, drawPos.x(), drawPos.y(), mValueTemp.c_str(),
                      nullptr, textBound);
        float lineh = textBound[3] - textBound[1];

        // find cursor positions
        int nglyphs =
            nvgTextGlyphPositions(ctx, drawPos.x(), drawPos.y(),
                                  mValueTemp.c_str(), nullptr, glyphs, maxGlyphs);
        updateCursor(ctx, textBound[2], glyphs, nglyphs);

        // compute text offset
        int prevCPos = mCursorPos > 0 ? mCursorPos - 1 : 0;
        int nextCPos = mCursorPos < nglyphs ? mCursorPos + 1 : nglyphs;
        float prevCX = cursorIndex2Position(prevCPos, textBound[2], glyphs, nglyphs);
        float nextCX = cursorIndex2Position(nextCPos, textBound[2], glyphs, nglyphs);

        if (nextCX > clipX + clipWidth)
            mTextOffset -= nextCX - (clipX + clipWidth) + 1;
        if (prevCX < clipX)
            mTextOffset += clipX - prevCX + 1;

        drawPos.x() = oldDrawPos.x() + mTextOffset;

        // draw text with offset
        nvgText(ctx, drawPos.x(), drawPos.y(), mValueTemp.c_str(), nullptr);
        nvgTextBounds(ctx, drawPos.x(), drawPos.y(), mValueTemp.c_str(),
                      nullptr, textBound);

        // recompute cursor positions
        nglyphs = nvgTextGlyphPositions(ctx, drawPos.x(), drawPos.y(),
                mValueTemp.c_str(), nullptr, glyphs, maxGlyphs);

        if (mCursorPos > -1) {
            if (mSelectionPos > -1) {
                float caretx = cursorIndex2Position(mCursorPos, textBound[2],
                                                    glyphs, nglyphs);
                float selx = cursorIndex2Position(mSelectionPos, textBound[2],
                                                  glyphs, nglyphs);

                if (caretx > selx)
                    std::swap(caretx, selx);

                // draw selection
                nvgBeginPath(ctx);
                nvgFillColor(ctx, nvgRGBA(160, 255, 160, 160));
                nvgRect(ctx, caretx, drawPos.y() - lineh * 0.5f, selx - caretx,
                        lineh);
                nvgFill(ctx);
            }

            float caretx = cursorIndex2Position(mCursorPos, textBound[2], glyphs, nglyphs);

            // draw cursor
            nvgBeginPath(ctx);
            nvgMoveTo(ctx, caretx, drawPos.y() - lineh * 0.5f);
            nvgLineTo(ctx, caretx, drawPos.y() + lineh * 0.5f);
            nvgStrokeColor(ctx, nvgRGBA(255, 192, 0, 255));
            nvgStrokeWidth(ctx, 1.0f);
            nvgStroke(ctx);
        }

 	   nvgFillColor(ctx,
                 mEnabled ? mTheme->mTextColor : mTheme->mDisabledTextColor);
        // draw text with offset
        nvgText(ctx, drawPos.x(), drawPos.y(), mValueTemp.c_str(), nullptr);
        nvgTextBounds(ctx, drawPos.x(), drawPos.y(), mValueTemp.c_str(),
                      nullptr, textBound);

    }
    nvgRestore(ctx);
	if (mSelected) drawSelectionBorder(ctx, mPos, mSize);

}

void EditorTextBox::loadProperties(PropertyFormHelper* properties) {
	EditorWidget::loadProperties(properties);
	nanogui::Widget *w = dynamic_cast<nanogui::Widget*>(this);
	if (w) {
		{
			nanogui::TextBox *tb = dynamic_cast<nanogui::TextBox* >(this);
			if (tb) {
				properties->addVariable<std::string> (
					"Text",
					[&,tb](std::string value) { tb->setValue(value); },
					[&,tb]()->std::string{ return tb->value(); });
				properties->addVariable<int> (
					"Alignment",
					[&,tb](int value) { tb->setAlignment((Alignment)value); },
					[&,tb]()->int{ return (int)tb->alignment(); });
			}
		}
		{
		nanogui::FloatBox<double> *tb = dynamic_cast<nanogui::FloatBox<double>* >(this);
		if (tb) {
			properties->addVariable<std::string> (
				"Number format",
				[&,tb](std::string value) { tb->numberFormat(value); },
				[&,tb]()->std::string{ return tb->numberFormat(); });
		}
		}
		properties->addGroup("Remote");
		properties->addVariable<std::string> (
			"Remote object",
			[&,this,properties](std::string value) {
				LinkableProperty *lp = EDITOR->gui()->findLinkableProperty(value);
				if (remote) remote->unlink(this);
				remote = lp;
				if (lp) { lp->link(new LinkableNumber(this)); }
			 },
			[&]()->std::string{ return remote ? remote->tagName() : ""; });
		properties->addVariable<std::string> (
			"Connection",
			[&,this,properties](std::string value) {
				if (remote) remote->setGroup(value); else setConnection(value);
			 },
			[&]()->std::string{ return remote ? remote->group() : getConnection(); });
	}
}

void EditorImageView::loadProperties(PropertyFormHelper* properties) {
	EditorWidget::loadProperties(properties);
	nanogui::Widget *w = dynamic_cast<nanogui::Widget*>(this);
	if (w) {
		properties->addVariable<std::string> (
			"Image File",
			[&,properties](std::string value) mutable{ setImageName(value, true); fit(); },
			[&,properties]()->std::string{ return imageName(); });
		properties->addVariable<float> ("Scale",
									[&](float value) mutable{ setScale(value); center(); },
									[&]()->float { return scale();  });
		properties->addGroup("Remote");
		properties->addVariable<std::string> (
			"Remote object",
			[&,this,properties](std::string value) {
				LinkableProperty *lp = EDITOR->gui()->findLinkableProperty(value);
				if (remote) remote->unlink(this);
				remote = lp;
				if (lp) { lp->link(new LinkableText(this)); }
			 },
			[&]()->std::string{ return remote ? remote->tagName() : ""; });
		properties->addVariable<std::string> (
			"Connection",
			[&,this,properties](std::string value) {
				if (remote) remote->setGroup(value); else setConnection(value);
			 },
			[&]()->std::string{ return remote ? remote->group() : getConnection(); });
	}
}

void EditorLabel::loadProperties(PropertyFormHelper* properties) {
	EditorWidget::loadProperties(properties);
	nanogui::Widget *w = dynamic_cast<nanogui::Widget*>(this);
	if (w) {
		properties->addVariable<std::string> (
			"Caption",
			[&](std::string value) mutable{ setCaption(value); },
			[&]()->std::string{ return caption(); });
		properties->addGroup("Remote");
		properties->addVariable<std::string> (
			"Remote object",
			[&,this,properties](std::string value) {
				LinkableProperty *lp = EDITOR->gui()->findLinkableProperty(value);
				if (remote) remote->unlink(this);
				remote = lp;
				if (lp) { lp->link(new LinkableText(this)); }
				//properties->refresh();
			 },
			[&]()->std::string{ return remote ? remote->tagName() : ""; });
		properties->addVariable<std::string> (
			"Connection",
			[&,this,properties](std::string value) {
				if (remote) remote->setGroup(value); else setConnection(value);
			 },
			[&]()->std::string{ return remote ? remote->group() : getConnection(); });
	}
}

void EditorProgressBar::loadProperties(PropertyFormHelper* properties) {
	EditorWidget::loadProperties(properties);
	nanogui::Widget *w = dynamic_cast<nanogui::Widget*>(this);
	if (w) {
		properties->addVariable<float> (
			"Value",
			[&](float value) mutable { setValue(value); },
			[&]()->float{ return value(); });
		properties->addGroup("Remote");
		properties->addVariable<std::string> (
			"Remote object",
			[&, w, this,properties](std::string value) mutable {
				LinkableProperty *lp = EDITOR->gui()->findLinkableProperty(value);
				if (remote) remote->unlink(this);
				remote = lp;
				if (lp) { lp->link(new LinkableNumber(this)); }
				//properties->refresh();
			 },
			[&]()->std::string{ return remote ? remote->tagName() : ""; });
		properties->addVariable<std::string> (
			"Connection",
			[&,this,properties](std::string value) {
				if (remote) remote->setGroup(value); else setConnection(value);
			 },
			[&]()->std::string{ return remote ? remote->group() : getConnection(); });
	}
}

void EditorButton::loadProperties(PropertyFormHelper* properties) {
	EditorWidget::loadProperties(properties);
	nanogui::Button *btn = dynamic_cast<nanogui::Button*>(this);
	if (btn) {
		EditorGUI *gui = EDITOR->gui();
		properties->addVariable<nanogui::Color> (
			"Off colour",
			 [&,btn](const nanogui::Color &value) mutable{ setBackgroundColor(value); },
			 [&,btn]()->const nanogui::Color &{ return backgroundColor(); });
		properties->addVariable<nanogui::Color> (
	 		"On colour",
	 			 [&,btn](const nanogui::Color &value) mutable{ setOnColor(value); },
	 			 [&,btn]()->const nanogui::Color &{ return onColor(); });
	 	properties->addVariable<nanogui::Color> (
	 		"Off text colour",
	 			[&,btn](const nanogui::Color &value) mutable{ setTextColor(value); },
	 			[&,btn]()->const nanogui::Color &{ return textColor(); } );
		properties->addVariable<nanogui::Color> (
			"On text colour",
			[&,btn](const nanogui::Color &value) mutable{ setOnTextColor(value); },
			[&,btn]()->const nanogui::Color &{ return onTextColor(); } );
		properties->addVariable<std::string> (
			"Off caption",
			[&](std::string value) mutable{ setCaption(value); },
			[&]()->std::string{ return caption(); });
			properties->addVariable<std::string> (
			"On caption",
				[&](std::string value) mutable{ setOnCaption(value); },
				[&]()->std::string{ return onCaption(); });
		properties->addVariable<int> (
			"Icon",
			[&](int value) mutable{ setIcon(value); },
			[&]()->int{ return icon(); });
		properties->addVariable<int> (
			"IconPosition",
			[&](int value) mutable{
				nanogui::Button::IconPosition icon_pos(nanogui::Button::IconPosition::Left);
				switch(value) {
					case 0: break;
					case 1: icon_pos = nanogui::Button::IconPosition::LeftCentered; break;
					case 2: icon_pos = nanogui::Button::IconPosition::RightCentered; break;
					case 3: icon_pos = nanogui::Button::IconPosition::Right; break;
					case 4: icon_pos = nanogui::Button::IconPosition::Filled; break;
					default: break;
				}
				setIconPosition(icon_pos);
				},
			[&]()->int{
				switch(iconPosition()) {
					case nanogui::Button::IconPosition::Left: return 0;
					case nanogui::Button::IconPosition::LeftCentered: return 1;
					case nanogui::Button::IconPosition::RightCentered: return 2;
					case nanogui::Button::IconPosition::Right: return 3;
					case nanogui::Button::IconPosition::Filled: return 4;
				}
				return 0;
			});
		properties->addVariable<unsigned int> (
			"Behaviour",
			[&](unsigned int value) mutable{ setFlags(value & 0xff); },
			[&]()->unsigned int{ return flags(); });
		properties->addVariable<std::string> (
			"Command",
			[&](std::string value) { setCommand(value); },
			[&]()->std::string{ return command(); });
		properties->addVariable<bool> (
			"Pushed",
			[&](bool value) { setPushed(value); },
			[&]()->bool{ return pushed(); });
		properties->addGroup("Remote");
		properties->addVariable<std::string> (
			"Remote object",
			[&,btn,this,properties](std::string value) {
				LinkableProperty *lp = EDITOR->gui()->findLinkableProperty(value);
				if (this->remote) this->remote->unlink(this);
				this->setRemote(lp);
				if (lp &&getDefinition()->getKind() == "INDICATOR")
					lp->link(new LinkableIndicator(this));
				//properties->refresh();
			 },
			[&]()->std::string{ return remote ? remote->tagName() : ""; });
		properties->addVariable<std::string> (
			"Connection",
			[&,this,properties](std::string value) {
				if (remote) remote->setGroup(value); else setConnection(value);
			 },
			[&]()->std::string{ return remote ? remote->group() : getConnection(); });
	}
}
void EditorLinePlot::setTriggerValue(UserWindow *user_window, SampleTrigger::Event evt, int val) {
	if (evt == SampleTrigger::START) start_trigger_value = val;
	else if (evt == SampleTrigger::STOP) stop_trigger_value = val;
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
	properties->addVariable<float> ("Grid Intensity",
		[&](float value) mutable{ grid_intensity = value; },
		[&]()->float { return grid_intensity; });
	properties->addVariable<bool> ("Display Grid",
		[&](bool value) mutable{ display_grid = value; },
		[&]()->bool { return display_grid; });
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
	properties->addButton("Save Data", [&,gui,this]() {
		std::string file_path( nanogui::file_dialog(
				{ {"csv", "Comma separated values"},
				  {"txt", "Text file"} }, true));
		if (file_path.length())
			this->saveData(file_path);
	});
	properties->addButton("Clear Data", [&]() {
		for (auto *ts : data) {
			ts->getData()->clear();
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
		properties->addGroup("Remote");
		properties->addVariable<std::string> (
			"Remote object",
			[&](std::string value) { /*setName(value);*/ },
			[&]()->std::string{ LinkableProperty *lp = EDITOR->gui()->findLinkableProperty(series->getName()); return lp ? lp->tagName() : ""; });
		properties->addVariable<std::string> (
			"Connection",
			[&,this,properties](std::string value) {
				if (remote) remote->setGroup(value); else setConnection(value);
			 },
			[&]()->std::string{ return remote ? remote->group() : getConnection(); });
	}
	properties->addGroup("Triggers");
	properties->addVariable<std::string> (
			"Start trigger",
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
	if (!buf) return;
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

void ObjectWindow::loadItems(const std::string group, const std::string match) {
	using namespace nanogui;
	clearSelections();
	std::cout << "loading object window items for group " << group << "\n";
	Widget *container = layers[group];
	if (!container || container->childCount() == 0) {
		std::cout << "object window has no tab '" << group << "', nothing to do";
		return;
	}
	Widget *scroller = container->childAt(0);
	VScrollPanel *vs = dynamic_cast<VScrollPanel*>(scroller);
	assert(vs);
	if (!scroller || scroller->childCount() == 0) {
		std::cout << "object tab has no content, nothing to do";
		return;
	}
	Widget *palette_content = scroller->childAt(0);
	if (!palette_content) return;
	while (palette_content && palette_content->childCount() > 0) {
		palette_content->removeChild(0);
	}

	std::vector<std::string> tokens;
	boost::algorithm::split(tokens, match, boost::is_any_of(", "));

	int n = tokens.size();
	std::cout << "filtering " << gui->getLinkableProperties().size() << " on " << n << " tokens: ";
	for (auto s : tokens) std::cout << s << " ";
	std::cout << "\n";

	bool within_group = group.length() > 0;
	for (auto item : gui->getLinkableProperties() ) {
		if (within_group && item.second->group() != group) {
			std::cout << "skipping " << item.first << " (group " << item.second->group() << ")";
			continue;
		}
		for (auto nam : tokens) {
			if (n>1 && nam.length() == 0) continue; //skip empty elements in a list
			if (nam.length() == 0 || item.first.find(nam) != std::string::npos) {
				Widget *cell = new Widget(palette_content);
				LinkableProperty *lp = item.second;
				cell->setFixedSize(Vector2i(window->width()-32,35));
				SelectableButton *b = new ObjectFactoryButton(gui, "BUTTON", this, cell, lp);
				b->setEnabled(true);
				b->setFixedSize(Vector2i(window->width()-40, 30));
				break; // only add once
			}
			else std::cout << "skipped: " << item.first << "\n";
		}
	}
	if (palette_content->childCount() == 0) {
		Widget *cell = new Widget(palette_content);
		cell->setFixedSize(Vector2i(window->width()-32,35));
		new Label(cell, "No match");
	}
	GridLayout *palette_layout = new GridLayout(Orientation::Horizontal,1,Alignment::Fill);
	palette_layout->setSpacing(1);
	palette_layout->setMargin(4);
	palette_layout->setColAlignment(nanogui::Alignment::Fill);
	palette_layout->setRowAlignment(nanogui::Alignment::Fill);
	palette_content->setLayout(palette_layout);

	window->performLayout(gui->nvgContext());
}

bool ObjectWindow::importModbusInterface(const std::string file_name, std::istream &init,
										 nanogui::Widget *palette_content,
										 nanogui::Widget *container) {

	using namespace nanogui;

	if (!init.good()) return false;

	char buf[200];

	std::istringstream iss;
	std::string group_name = shortName(file_name);


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
		if (tag_name.length()) {
			LinkableProperty *lp = gui->findLinkableProperty(tag_name);
			if (lp == nullptr)
				lp = new LinkableProperty(group_name, kind, tag_name, address_str, data_type, data_count);
			else {
				const std::string &old_group(lp->group());
				int old_kind = lp->getKind();
				const std::string &old_addr_str(lp->addressStr());
				const CircularBuffer::DataType old_dt = lp->dataType();
				int old_size = lp->dataSize();
				lp->setGroup(group_name);
				lp->setKind(kind);
				lp->setAddressStr(address_str);
				lp->setDataTypeStr(data_type);
				lp->setDataSize(data_count);
			}
			gui->getUserWindow()->addDataBuffer(tag_name, CircularBuffer::dataTypeFromString(data_type), gui->getSampleBufferSize());
			gui->addLinkableProperty(tag_name, lp);
		}
/*
		cout << kind << " " << tag_name << " "
		<< data_type << " " << data_count << " " << address_str
		<< "\n";
*/
	}
	loadItems(group_name, search_box->value());

	return true;
}

bool applyWindowSettings(Structure *item, nanogui::Widget *widget) {
	if (widget) {
		nanogui::Screen *screen = dynamic_cast<nanogui::Screen*>(widget);
		{
			const Value &vw(item->getProperties().find("w"));
			const Value &vh(item->getProperties().find("h"));
			long w, h;
			if (vw.asInteger(w) && vh.asInteger(h)) {
				if (screen) {
					screen->setSize(nanogui::Vector2i(w, h));
					//std::cout << item->getName() << " screen size: " << w << "," << h << "\n";
				}
				else {
					widget->setSize(nanogui::Vector2i(w, h));
					widget->setFixedSize(nanogui::Vector2i(w, h));
					//std::cout << item->getName() << " size: " << w << "," << h << "\n";
				}
			}
		}
		{
			const Value &vx(item->getProperties().find("x"));
			const Value &vy(item->getProperties().find("y"));
			long x, y;
			if (vx.asInteger(x) && vy.asInteger(y)) {
				if (screen)
					screen->setPosition(nanogui::Vector2i(x, y));
				else {
					nanogui::Vector2i pos(x,y);
					pos = fixPositionInWindow(pos, widget->size(), widget->parent()->size());
					//std::cout << item->getName() << " position: " << pos.x() << "," << pos.y() << "\n";

					widget->setPosition(pos);
				}
			}
		}
		{
			SkeletonWindow *skel = dynamic_cast<SkeletonWindow*>(widget);
			if (skel) {
				const Value &sx(item->getProperties().find("sx")); // position when shrunk
				const Value &sy(item->getProperties().find("sy")); // position when shrunk
				long x, y;
				if (sx.asInteger(x) && sy.asInteger(y)) {
					nanogui::Vector2i pos(x, y);
					pos = fixPositionInWindow(pos, widget->size(), widget->parent()->size());
					//std::cout << item->getName() << " shrunk position: " << pos.x() << "," << pos.y() << "\n";

					skel->setShrunkPos(pos);
				}
			}
		}

		long vis = 0;
		const Value &vis_prop(item->getProperties().find("visible"));
		if (vis_prop != SymbolTable::Null && vis_prop.asInteger(vis))
		{
			if (!vis) std::cout << item->getName() << " is invisible\n";
			EDITOR->gui()->getViewManager().set(item->getName(), vis);
		}
		else
			EDITOR->gui()->getViewManager().set(item->getName(), true);
		return true;
	}
	else return false;
}

bool updateSettingsStructure(const std::string name, nanogui::Widget *widget) {
	if (!widget) return false;
	SkeletonWindow *skel = dynamic_cast<SkeletonWindow *>(widget);

	EditorSettings::setDirty();
	Structure *s = EditorSettings::find(name);
	if (!s) {
		s = new Structure(nullptr, name, "WINDOW");
		st_structures.push_back(s);
		std::cout << "added structure for window " << name << "\n";
	}
	const nanogui::Vector2i &pos(widget->position());
	SymbolTable &properties(s->getProperties());
	if (skel && skel->isShrunk()) {
		properties.add("sx", pos.x());
		properties.add("sy", pos.y());
		skel->setShrunkPos(pos);
		std::cout << "set shrunk position to " << pos.x() << "," << pos.y() << "\n";
	}
	else {
		properties.add("x", pos.x());
		properties.add("y", pos.y());
		properties.add("w", widget->width());
		properties.add("h", widget->height());
	}
	if (!EDITOR->gui()->getViewManager().get(name).visible)
		properties.add("visible", 0);
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

	//std:cout << "running from the " << boost::filesystem::current_path().string() << " directory\n";
	//std::string home_path(getenv("HOME"));
	//assert(boost::filesystem::current_path().string().find(home_path) == 0);

	zmq::context_t context;
	MessagingInterface::setContext(&context);

	Logger::instance();
	Logger::instance()->setLevel(Logger::Debug);
	//LogState::instance()->insert(DebugExtra::instance()->DEBUG_CHANNELS);

	int cw_port = 5555;
	std::string hostname;
	//std::string tag_file_name;

	setup_signals();

	po::options_description generic("Commandline options");
	generic.add_options()
	("help", "produce help message")
	("debug",po::value<int>(&debug)->default_value(0), "set debug level")
	("host", po::value<std::string>(&hostname)->default_value("localhost"),"remote host (localhost)")
	("cwport",po::value<int>(&cw_port)->default_value(5555), "clockwork port (5555)")
	("tags", po::value<std::string>(&tag_file_name)->default_value(""),"clockwork tag file")
	("full_screen",po::value<long>(&full_screen_mode)->default_value(0), "full screen")
	("run_only", po::value<int>(&run_only)->default_value(0), "run only (default 0)")
	;
	po::options_description hidden("Hidden options");
	hidden.add_options()
    ("source-file", po::value< std::vector<std::string> >(), "source file")
    ;
	po::options_description cmdline_options;
	cmdline_options.add(generic).add(hidden);

	po::positional_options_description p;
	p.add("source-file", -1);

	po::variables_map vm;
	try {
		po::store(po::command_line_parser(argc, argv).
		options(cmdline_options).positional(p).run(), vm);
		po::notify(vm);
	}
	catch (std::exception e) {
		std::cerr << e.what() << "\n";
		exit(2);
	}

	if (vm.count("help")) {
		std::cout << generic << "\n";
		return 1;
	}

	if (vm.count("cwout")) cw_port = vm["cwout"].as<int>();
	if (vm.count("host")) hostname = vm["host"].as<std::string>();
	if (vm.count("debug")) debug = vm["debug"].as<int>();
	if (vm.count("tags")) tag_file_name = vm["tags"].as<std::string>();
	if (vm.count("run_only")) run_only = vm["run_only"].as<int>();
	if (DEBUG_BASIC) std::cout << "Debugging\n";

	std::string home(".");
	char *home_env = nullptr;
	if ( (home_env = getenv("HOME")) != nullptr )
		home = home_env;
	std::string fname(home);
	fname += "/.humidrc";
	settings_files.push_back(fname);
	loadSettingsFiles(settings_files);
	for (auto item : st_structures) {
		std::cout << "Loaded settings item: " << item->getName() << " : " << item->getKind() << "\n";
		structures[item->getName()] = item;
	}
	Structure *es = EditorSettings::find("EditorSettings");
	if (!es)
		es = EditorSettings::create();

	gettimeofday(&start, 0);

	try {
		nanogui::init();

		GLFWmonitor* primary = glfwGetPrimaryMonitor();
		const GLFWvidmode* mode = glfwGetVideoMode(primary);
		int widthMM, heightMM;
		glfwGetMonitorPhysicalSize(primary, &widthMM, &heightMM);
		const double dpi = mode->width / (widthMM / 25.4);

		{
			if (vm.count("source-file")) {
				const std::vector<std::string> &files( vm["source-file"].as< std::vector<std::string> >() );
				for (auto s : files) {
					int found = s.rfind("/");
					if (found == s.length()-1) s.erase(found);
					source_files.push_back(s);
				}
				loadProjectFiles(source_files);
			}
			StructureClass *system_class = findClass("SYSTEM");
			if (!system_class) {
				system_class = new StructureClass("SYSTEM", "");
				hm_classes.push_back(system_class);
			}
			system_settings = findStructure("System");
			if (!system_settings) {
				system_settings = system_class->instantiate(nullptr, "System");
			}

			// if necessary create a project settings structure to store the 
			// nominated connection details
			Structure *project_settings = findStructure("ProjectSettings");
			if (!project_settings) {
				StructureClass *psc = new StructureClass("PROJECTSETTINGS", "");
				hm_classes.push_back(psc);
				project_settings = psc->instantiate(nullptr, "ProjectSettings");
				if (cw_port && hostname.length()) { // user supplied a host, setup the required connection object
					Structure *conn = new Structure(nullptr, "Remote", "CONNECTION");
					conn->getProperties().add("host", hostname.c_str());
					conn->getProperties().add("port", cw_port);
					Parameter p(conn->getName());
					p.machine = conn;
					psc->addLocal(p);
				}
			}


			Value full_screen_v = system_settings->getProperties().find("full_screen");
			long full_screen = 1;
			full_screen_v.asInteger(full_screen);
			if (vm.count("full_screen")) full_screen = vm["full_screen"].as<long>();

			long width = mode->width;
			long height = mode->height;
			std::cout << "intial videomode: " << width << "x" << height << "\n";
			{
				const Value width_v = system_settings->getProperties().find("w");
				const Value height_v = system_settings->getProperties().find("h");
				width_v.asInteger(width);
				height_v.asInteger(height);
			}
			std::cout << "settings videomode: " << width << "x" << height << " fullscreen:" << full_screen << "\n";

			nanogui::ref<EditorGUI> app = (full_screen)
					? new EditorGUI(width, height, full_screen != 0)
					: new EditorGUI(width, height);
			nanogui::Theme *myTheme = new nanogui::Theme(app->nvgContext());
			setupTheme(myTheme);
			app->setTheme(myTheme);

			app->createWindows();

			Value remote_screen(system_settings->getProperties().find("remote_screen"));
			if (!system_settings->getStructureDefinition()) {
				system_settings->setStructureDefinition(findClass("SYSTEM"));
			}

			if (remote_screen == SymbolTable::Null) {
				system_settings->getProperties().add("remote_screen", Value("P_Screen", Value::t_string));
				remote_screen = system_settings->getProperties().find("remote_screen");
			}
			else {
				LinkableProperty *lp = app->findLinkableProperty(remote_screen.asString());
				if (lp) lp->link(app->getUserWindow());
			}
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
