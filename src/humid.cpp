/*
	All rights reserved. Use of this source code is governed by the
	3-clause BSD License in LICENSE.txt.
*/
/* This file is formatted with:
    astyle --style=kr --indent=tab=2 --one-line=keep-blocks --brackets=break-closing
*/
#include <nanogui/screen.h>
#include <nanogui/window.h>
#include <nanogui/messagedialog.h>
#include <nanogui/vscrollpanel.h>
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
#include "propertymonitor.h"
#include "draghandle.h"
#include "skeleton.h"
#include "panelscreen.h"
#include "editorproject.h"
#include "editorsettings.h"
#include "editorgui.h"
#include "thememanager.h"

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
#include "palette.h"
#include "selectablebutton.h"
#include "userwindow.h"
#include "editor.h"
#include "structureswindow.h"
#include "objectwindow.h"
#include "patternswindow.h"
#include "themewindow.h"
#include "propertywindow.h"
#include "helper.h"
#include "screenswindow.h"
#include "toolbar.h"
#include "propertyformhelper.h"
#include "viewswindow.h"
#include "startupwindow.h"
#include "editorwidget.h"
#include "colourhelper.h"

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

long full_screen_mode = 0;
int run_only = 0;

extern long collect_history;

int num_errors = 0;
std::list<std::string>error_messages;
std::list<std::string>settings_files;
std::list<std::string> source_files;

using std::cout;
using std::cerr;
using std::endl;
using std::locale;
using nanogui::Vector2i;
using nanogui::Vector2f;
using nanogui::Vector2d;
using nanogui::MatrixXd;
using nanogui::Matrix3d;

namespace po = boost::program_options;

extern const int DEBUG_ALL;
#define DEBUG_BASIC ( 1 & debug)
extern int debug;
extern int saved_debug;
std::string tag_file_name;

extern const char *program_name;

//extern int cw_out;
//extern std::string host;
extern const char *local_commands;
extern ProgramState program_state;
extern struct timeval start;

extern void setup_signals();

std::string stripEscapes(const std::string &s);
//StructureClass *findClass(const std::string &name);

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
		item->setPosition(nanogui::Vector2i(x,y));
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


bool loadProjectFiles(std::list<std::string> &files_and_directories) {
	using namespace boost::filesystem;

	Structure *settings = EditorSettings::find("EditorSettings");
	if (!settings) settings = EditorSettings::create();
	assert(settings);
	bool base_checked = false;

	std::list<path> files;
	{
		std::list<std::string> errors;
		std::list<std::string>::iterator fd_iter = files_and_directories.begin();
		while (fd_iter != files_and_directories.end()) {
			path fp = (*fd_iter++).c_str();
			if (!exists(fp)) {
				std::stringstream error;
				error << "file/directory does not exist: " << fp.c_str();
				errors.push_back(error.str());
			}
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
		if (!errors.empty()) {
			std::cerr << "Errors during load; cannot continue\n";
			for (const auto & error : errors) { std::cerr << error << "\n"; }
			return false;
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
					continue; // already loaded this one-line
				}
				loaded_files.insert(fname);
			}
			if (exists(filename)) {
				opened_file = 1;
				yyin = fopen(filename, "r");
				if (yyin)
				{
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
	return true;

}


nanogui::Vector2i fixPositionInWindow(const nanogui::Vector2i &pos, const nanogui::Vector2i &siz, const nanogui::Vector2i &area);

CircularBuffer *UserWindow::getDataBuffer(const std::string item) {
	std::map<std::string, CircularBuffer *>::iterator found = data.find(item);
	if (found == data.end()) return nullptr;
	//	addDataBuffer(item, gui->sampleBufferSize());
	return (*found).second;
}

bool applyWindowSettings(Structure *item, nanogui::Widget *widget) {
	if (widget) {
		nanogui::Screen *screen = dynamic_cast<nanogui::Screen*>(widget);
		if (item->getName() != "Structures") {
			const Value &vw(item->getProperties().find("w"));
			const Value &vh(item->getProperties().find("h"));
			long w, h;
			if (vw.asInteger(w) && vh.asInteger(h)) {
				if (screen) {
					screen->setSize(nanogui::Vector2i(w, h));
					std::cout << item->getName() << " screen size: " << w << "," << h << "\n";
				}
				else {
					widget->setSize(nanogui::Vector2i(w, h));
					widget->setFixedSize(nanogui::Vector2i(w, h));
					std::cout << item->getName() << " size: " << w << "," << h << "\n";
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

	Structure *s = EditorSettings::find(name);
	if (!s) {
		s = new Structure(nullptr, name, "WINDOW");
		st_structures.push_back(s);
		EditorSettings::setDirty();
	}
	const nanogui::Vector2i &pos(widget->position());
	SymbolTable &properties(s->getProperties());
	if (skel && skel->isShrunk()) {
		properties.add("sx", pos.x());
		properties.add("sy", pos.y());
		skel->setShrunkPos(pos);
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
	Structure::loadBuiltins();
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
				if (!loadProjectFiles(source_files)) {
					return EXIT_FAILURE;
				}
				auto errors = checkStructureClasses();
				if (!errors.empty()) {
					std::cerr << "Errors detected:\n";
					for (const auto error : errors) {
						std::cerr << "    " << error << "\n";
					}
					return EXIT_FAILURE;
				}
			}
			StructureClass *system_class = findClass("SYSTEM");
			if (!system_class) {
				system_class = new StructureClass("SYSTEM", "");
				hm_classes.push_back(system_class);
			}
			EditorGUI::systemSettings(findStructure("System"));
			if (!EditorGUI::systemSettings()) {
				EditorGUI::systemSettings(system_class->instantiate(nullptr, "System"));
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
				psc->getProperties().add("asset_path", Value(".", Value::t_string));
			}
			if (!project_settings->getStructureDefinition()) {
				project_settings->setStructureDefinition(findClass("PROJECTSETTINGS"));
			}
			auto asset_path = project_settings->getStringProperty("asset_path");
			if (asset_path.empty()) {
				project_settings->getProperties().add("asset_path", Value(".", Value::t_string));
				project_settings->setChanged(true);
			}

			Value full_screen_v = EditorGUI::systemSettings()->getProperties().find("full_screen");
			long full_screen = 1;
			full_screen_v.asInteger(full_screen);
			if (vm.count("full_screen")) full_screen = vm["full_screen"].as<long>();

			long width = mode->width;
			long height = mode->height;
			std::cout << "intial videomode: " << width << "x" << height << "\n";
			{
				const Value width_v = EditorGUI::systemSettings()->getProperties().find("main_window_width");
				const Value height_v = EditorGUI::systemSettings()->getProperties().find("main_window_height");
				width_v.asInteger(width);
				height_v.asInteger(height);
			}
			if (run_only) {
				const Value width_v = EditorGUI::systemSettings()->getProperties().find("panel_width");
				const Value height_v = EditorGUI::systemSettings()->getProperties().find("panel_height");
				width_v.asInteger(width);
				height_v.asInteger(height);
			}
			
			std::cout << "settings videomode: " << width << "x" << height << " fullscreen:" << full_screen << "\n";

			nanogui::ref<EditorGUI> app = (full_screen)
					? new EditorGUI(width, height, full_screen != 0)
					: new EditorGUI(width, height);
			ThemeManager::instance().setContext(app->nvgContext());
			for (auto settings : Structure::findStructureClasses("THEME")) {
				ThemeManager::instance().addTheme(settings->getName(), ThemeManager::instance().createTheme(settings));
			}
			if (auto main_theme = ThemeManager::instance().findTheme("EditorTheme")) {
				app->setTheme(main_theme);
			}
			else {
				main_theme = ThemeManager::instance().createTheme();
				ThemeManager::instance().addTheme("EditorTheme", main_theme);
				app->setTheme(main_theme);
			}

			app->createWindows();

			if (!EditorGUI::systemSettings()->getStructureDefinition()) {
				EditorGUI::systemSettings()->setStructureDefinition(findClass("SYSTEM"));
			}

			Value remote_screen(EditorGUI::systemSettings()->getProperties().find("remote_screen"));
			if (remote_screen == SymbolTable::Null) {
				EditorGUI::systemSettings()->getProperties().add("remote_screen", Value("P_Screen", Value::t_string));
				remote_screen = EditorGUI::systemSettings()->getProperties().find("remote_screen");
			}
			{
				LinkableProperty *lp = app->findLinkableProperty(remote_screen.asString());
				if (lp) {
					lp->link(app->getUserWindow());
				}
			}
			Value remote_dialog(EditorGUI::systemSettings()->getProperties().find("remote_dialog"));
			if (remote_dialog != SymbolTable::Null) {
				LinkableProperty *lp = app->findLinkableProperty(remote_dialog.asString());
				if (lp) {
					std::cout << "WARNING: should not do this\n";
					//lp->link(app->getUserWindow());
				}
			}
			app->setVisible(true);

			if (!app->getUserWindow()->structure()) {
				const Value &active(EditorGUI::systemSettings()->getProperties().find("active_screen"));
				if (active == SymbolTable::Null) {
					app->getScreensWindow()->selectFirst();
				}
				else {
					app->getScreensWindow()->select(active.asString());
				}
			}

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
