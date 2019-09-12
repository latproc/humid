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
#include <string>
#include <vector>
#include <set>
#include <functional>
#include <libgen.h>
#include <boost/thread.hpp>
#include <boost/thread/condition.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>
#include <lib_clockwork_client.hpp>
#include "includes.hpp"
#include "list_panel.h"
#include "userwindowwin.h"
#include "toolbar.h"
#include "startupwindow.h"
#include "viewswindow.h"

#ifndef ENTYPO_ICON_LAYOUT
#define ENTYPO_ICON_LAYOUT                              0x0000268F
#endif

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
std::list<std::string> error_messages;
std::list<std::string> settings_files;
std::list<std::string> source_files;

using std::cout;
using std::cerr;
using std::endl;
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

extern const char *local_commands;
extern ProgramState program_state;
extern struct timeval start;

#ifndef _WIN32
extern void setup_signals();
#endif

std::string stripEscapes(const std::string &s);
StructureClass *findClass(const std::string &name);
nanogui::Vector2i fixPositionInWindow(const nanogui::Vector2i &pos, const nanogui::Vector2i &siz, const nanogui::Vector2i &area);

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


void loadProjectFiles(std::list<std::string> &files_and_directories) {
	using namespace boost::filesystem;
	if (files_and_directories.empty())
		return;

	// if a file was given, use its parent directory as the base
	boost::filesystem::path base_path(files_and_directories.front());
	if (!boost::filesystem::is_directory(base_path.string()) && base_path.has_parent_path()) {
		base_path = base_path.parent_path();
	}
	assert(boost::filesystem::is_directory(base_path.string())); // what should we do here?
	bool base_checked = false;

	// Set the directory as the project base, subsequent file references will all be relative to this base.
	Structure *settings = EditorSettings::find("EditorSettings");
	if (!settings)
	{
		std::cout << "## not found - EditorSettings\n\tcreating..\n";
		settings = EditorSettings::create();
	}
	assert(settings);
	settings->getProperties().add("project_base", Value(base_path.string(), Value::t_string));

	std::list<path> files;
	{
		std::list<std::string>::iterator fd_iter = files_and_directories.begin();
		while (fd_iter != files_and_directories.end()) {
			path fp = (*fd_iter++).c_str();
			if (!exists(fp))
            {
                std::cout << "## - Not Found " << fp << "\n";
                continue;
            }
            else
            {
                // std::cout << "++ - Found " << fp << "\n";
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
	}

	/* load configuration from given files */
	int opened_file = 0;
	// std::string base(base_path.string());
	std::string base = base_path.string();
	std::set<std::string>loaded_files;
	std::list<path>::const_iterator f_iter = files.begin();
    // std::cout << "- Loading Files\n";
	while (f_iter != files.end())
	{
        boost::filesystem::path next_path = *f_iter++;
		const char *filename = next_path.string().c_str();
        std::string fname = next_path.string();
        // std::cout << "\t - file " << fname << "\n";
        // std::string fname(filename);

        // boost::filesystem::path path_fix(filename);
        // std::string fname = path_fix.string();

		if (filename[0] != '-')
		{
			// strip project path from the file name
			if (base.length() && fname.find(base) == 0) {

				// fname = fname.substr(base.length()+1);

				if (loaded_files.count(fname)) {
					// std::cout << "\t\t" << "Skipping second load of " << fname << "\n";
					continue; // already loaded this one-line
				}
				loaded_files.insert(fname);
			}
			if (exists(next_path)) {
				// std::cout << "\t\t" << "reading project file " << fname << "\n";
				opened_file = 1;
				yyin = fopen(fname.c_str(), "r");
				if (yyin)
				{
					// std::cerr << "\t\t" << "Processing file: " << fname << "\n";
					// std::cout << "\t\t" << "Processing file: " << fname << "\n";
					yylineno = 1;
					yycharno = 1;
					yyfilename = fname.c_str();
					yyparse();
					fclose(yyin);
				}
				else
				{
					std::stringstream ss;
					ss << "\t\t" << "## - Error: failed to load project file: " << fname;
                    std::cout << "\t\t" << "## - Error: failed to load project file: " << fname << "\n";
					error_messages.push_back(ss.str());
					++num_errors;
				}
			}
            else
            {
                std::cout << "\t\t" << "## Not Found: " << fname << "\n";
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
	}
}

void EditorTextBox::loadProperties(PropertyFormHelper* properties) {
	EditorWidget::loadProperties(properties);
	nanogui::Widget *w = dynamic_cast<nanogui::Widget*>(this);
	if (w) {
		properties->addVariable<std::string> (
			"Text",
			[&](std::string value) { setProperty("Text", value); },
			[&]()->std::string{ return getPropertyValue("Text").asString(); });

		properties->addVariable<int> (
			"Alignment",
			[&](int value) { setAlignment((Alignment)value); },
			[&]()->int{ return (int)alignment(); });
		properties->addVariable<int> (
			"Vertical Alignment",
			[&](int value) mutable{ valign = value; },
			[&]()->int{ return valign; });
		properties->addVariable<bool> (
			"Wrap Text",
			[&](bool value) mutable{ wrap_text = value; },
			[&]()->bool{ return wrap_text; });
		properties->addGroup("Remote");
		properties->addVariable<std::string> (
			"Remote object",
			[&,this,properties](std::string value) {
				LinkableProperty *lp = EDITOR->gui()->findLinkableProperty(value);
        this->setRemoteName(value);
				if (remote) remote->unlink(this);
				remote = lp;
				if (lp) { lp->link(new LinkableNumber(this)); }
			 },
			[&]()->std::string{
				if (remote) return remote->tagName();
				if (getDefinition()) {
					const Value &rmt_v = getDefinition()->getProperties().find("remote");
					if (rmt_v != SymbolTable::Null)
						return rmt_v.asString();
				}
				return "";
			});
		properties->addVariable<std::string> (
			"Connection",
			[&,this,properties](std::string value) {
				if (remote) remote->setGroup(value); else setConnection(value);
			 },
			[&]()->std::string{ return remote ? remote->group() : getConnection(); });

		properties->addVariable<std::string> (
			"Visibility",
			[&,this,properties](std::string value) {
				LinkableProperty *lp = EDITOR->gui()->findLinkableProperty(value);
				if (visibility) visibility->unlink(this);
				visibility = lp;
				if (lp) { lp->link(new LinkableVisibility(this)); }
			 },
			[&]()->std::string{ return visibility ? visibility->tagName() : "";
		});
	}
}


bool updateSettingsStructure(const std::string name, nanogui::Widget *widget) {
	if (!widget) return false;
	const nanogui::Vector2i pos(widget->position());
    const nanogui::Vector2i size(widget->width(), widget->height());
    Shrinkable *skel = dynamic_cast<Shrinkable *>(widget);
    bool is_shrunk = skel && skel->isShrunk();
    if (is_shrunk) {
        skel->setShrunkPos(widget->position());
    }
	bool visible = EDITOR->gui()->getViewManager().get(name).visible;
    EditorSettings::updateWindowSettings(name, pos, size, visible, is_shrunk);
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
		const char *param = (*f_iter).c_str();
		if (param[0] != '-')
		{
			opened_file = 1;
            boost::filesystem::path path_fix(param);
            std::string filename = path_fix.string();
            const char* filename_cstr = filename.c_str();
			st_yyin = fopen(filename_cstr, "r");
			if (st_yyin)
			{
				// std::cerr << "Processing file: " << filename << "\n";
				st_yylineno = 1;
				st_yycharno = 1;
				st_yyfilename = filename_cstr;
				st_yyparse();
				fclose(st_yyin);
			}
			else
			{
				std::stringstream ss;
				ss << "## - Error: failed to load config: " << filename_cstr;
				error_messages.push_back(ss.str());
				++num_errors;
			}
		}
		else if (strlen(param) == 1) /* '-' means stdin */
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
#ifndef _WIN32
	setup_signals();
#endif
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
	catch (const std::exception &e) {
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
	if ((home_env = getenv("HOME")) != nullptr)
    {
		home = home_env;
    }
	// std::string fname(home);
	// fname += "/.humidrc";
	// settings_files.push_back(fname);
	loadSettingsFiles(settings_files);
	for (auto item : st_structures) {
		// std::cout << "Loaded settings item: " << item->getName() << " : " << item->getKind() << "\n";
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
                    std::cout << "Reading source-file: " << s << "\n";
					int found = s.rfind("/");
					if (found == s.length()-1)
                    {
                        s.erase(found);
                        std::cout << "\t edited path: " << s << "\n";
                    }
                    std::cout << "\t added path: " << s << "\n";
					source_files.push_back(s);
				}
			}
			loadProjectFiles(source_files);
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
			}


			Value full_screen_v = EditorGUI::systemSettings()->getProperties().find("full_screen");
			long full_screen = 1;
			full_screen_v.asInteger(full_screen);
			if (vm.count("full_screen"))
            {
                full_screen = vm["full_screen"].as<long>();
            }

			long width = mode->width;
			long height = mode->height;
			// std::cout << "intial videomode: " << width << "x" << height << "\n";
			{
				const Value width_v = EditorGUI::systemSettings()->getProperties().find("w");
				const Value height_v = EditorGUI::systemSettings()->getProperties().find("h");
				width_v.asInteger(width);
				height_v.asInteger(height);
			}
			// std::cout << "settings videomode: " << width << "x" << height << " fullscreen:" << full_screen << "\n";

			nanogui::ref<EditorGUI> app = (full_screen)
					? new EditorGUI(width, height, full_screen != 0)
					: new EditorGUI(width, height);
			nanogui::Theme *myTheme = new nanogui::Theme(app->nvgContext());
			setupTheme(myTheme);
			app->setTheme(myTheme);

			app->createWindows();

			Value remote_screen(EditorGUI::systemSettings()->getProperties().find("remote_screen"));
			if (!EditorGUI::systemSettings()->getStructureDefinition()) {
				EditorGUI::systemSettings()->setStructureDefinition(findClass("SYSTEM"));
			}

			if (remote_screen == SymbolTable::Null) {
				EditorGUI::systemSettings()->getProperties().add("remote_screen", Value("P_Screen", Value::t_string));
				remote_screen = EditorGUI::systemSettings()->getProperties().find("remote_screen");
			}
			else {
				LinkableProperty *lp = app->findLinkableProperty(remote_screen.asString());
				if (lp) lp->link(app->getUserWindow());
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
