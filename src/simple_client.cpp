/*
	All rights reserved. Use of this source code is governed by the
	3-clause BSD License in LICENSE.txt.
*/
/* This file is formatted with:
    astyle --style=kr --indent=tab=2 --one-line=keep-blocks --brackets=break-closing
*/
#include <nanogui/button.h>
#include <nanogui/checkbox.h>
#include <nanogui/colorwheel.h>
#include <nanogui/combobox.h>
#include <nanogui/entypo.h>
#include <nanogui/formhelper.h>
#include <nanogui/graph.h>
#include <nanogui/imagepanel.h>
#include <nanogui/imageview.h>
#include <nanogui/label.h>
#include <nanogui/layout.h>
#include <nanogui/messagedialog.h>
#include <nanogui/popupbutton.h>
#include <nanogui/progressbar.h>
#include <nanogui/screen.h>
#include <nanogui/slider.h>
#include <nanogui/textbox.h>
#include <nanogui/theme.h>
#include <nanogui/toolbutton.h>
#include <nanogui/vscrollpanel.h>
#include <nanogui/window.h>
#if defined(_WIN32)
#include <windows.h>
#endif
#include "draghandle.h"
#include "manuallayout.h"
#include "propertymonitor.h"
#include "skeleton.h"
#include <fstream>
#include <iostream>
#include <locale>
#include <nanogui/glutil.h>
#include <string>

#include <ConnectionManager.h>
#include <MessageEncoding.h>
#include <MessagingInterface.h>
#include <SocketMonitor.h>
#include <cJSON.h>
#include <libgen.h>
#include <signal.h>
#include <zmq.hpp>

#include <boost/program_options.hpp>
#include <boost/thread.hpp>
#include <boost/thread/condition.hpp>
#include <boost/thread/mutex.hpp>

using nanogui::Matrix3d;
using nanogui::MatrixXd;
using nanogui::Vector2d;
using nanogui::Vector2i;
using std::cerr;
using std::cout;
using std::endl;
using std::locale;

namespace po = boost::program_options;

extern const int DEBUG_ALL;
#define DEBUG_BASIC (1 & debug)
extern int debug;
extern int saved_debug;

const char *program_name;

extern int cw_out;
extern std::string host;
extern const char *local_commands;
extern ProgramState program_state;
extern struct timeval start;

extern void setup_signals();

class MyGUI : public ClockworkClient {
  public:
    MyGUI() : ClockworkClient(nanogui::Vector2i(1024, 768), "Clockwork Client") {

        using namespace nanogui;

        ConfirmDialog *s = new ConfirmDialog(this, "Hello");
        window = s->getWindow();

        performLayout(mNVGContext);
    }
};

int main(int argc, const char **argv) {
    char *pn = strdup(argv[0]);
    program_name = strdup(basename(pn));
    free(pn);

    zmq::context_t context;
    MessagingInterface::setContext(&context);

    int cw_port;
    std::string hostname;

    //program_state = s_initialising;
    setup_signals();

    po::options_description desc("Allowed options");
    desc.add_options()("help", "produce help message")(
        "debug", po::value<int>(&debug)->default_value(0),
        "set debug level")("host", po::value<std::string>(&hostname)->default_value("localhost"),
                           "remote host (localhost)")(
        "cwout", po::value<int>(&cw_port)->default_value(5555),
        "clockwork outgoing port (5555)")("cwin", "clockwork incoming port (deprecated)");
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.count("help")) {
        std::cout << desc << "\n";
        return 1;
    }

    if (vm.count("cwout"))
        cw_out = vm["cwout"].as<int>();
    if (vm.count("host"))
        host = vm["host"].as<std::string>();
    if (vm.count("debug"))
        debug = vm["debug"].as<int>();
    if (DEBUG_BASIC)
        std::cout << "Debugging\n";

    gettimeofday(&start, 0);

    try {
        nanogui::init();

        {
            nanogui::ref<MyGUI> app = new MyGUI();
            //app->drawAll();
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
