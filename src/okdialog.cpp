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
#if defined(_WIN32)
#include <windows.h>
#endif
#include <nanogui/glutil.h>
#include <iostream>
#include <fstream>
#include <locale>
#include <string>
#include "propertymonitor.h"
#include "draghandle.h"
#include "manuallayout.h"

#include <libgen.h>
#include <zmq.hpp>
#include <cJSON.h>
#include <MessageEncoding.h>
#include <MessagingInterface.h>
#include <signal.h>
#include <SocketMonitor.h>
#include <ConnectionManager.h>

#include <boost/thread.hpp>
#include <boost/thread/condition.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/program_options.hpp>


using std::cout;
using std::cerr;
using std::endl;
using std::locale;
using nanogui::Vector2i;
using nanogui::Vector2d;
using nanogui::MatrixXd;
using Einanoguigen::Matrix3d;

namespace po = boost::program_options;

const int DEBUG_ALL = 255;
#define DEBUG_BASIC ( 1 & debug)
static nanogui::DragHandle *drag_handle = 0;
int debug = DEBUG_ALL;
int saved_debug = 0;

/* Clockwork Interface */
const char *program_name;
enum ProgramState { s_initialising, s_running, s_finished } program_state = s_initialising;
boost::mutex update_mutex;
int cw_out = 5555;
std::string host("127.0.0.1");
const char *local_commands = "inproc://local_cmds";

static bool need_refresh = false; // not fully implemented yet

struct timeval start;

class SetupDisconnectMonitor : public EventResponder {

public:
	void operator()(const zmq_event_t &event_, const char* addr_) {
	}
};

class SetupConnectMonitor : public EventResponder {

public:
	void operator()(const zmq_event_t &event_, const char* addr_) {
		need_refresh = true;
	}
};

uint64_t get_diff_in_microsecs(struct timeval *now, struct timeval *then) {
	uint64_t t = (now->tv_sec - then->tv_sec);
	t = t * 1000000 + (now->tv_usec - then->tv_usec);
	return t;
}

/* Drag and Drop interface */
#define TO_STRING( ID ) #ID

static nanogui::Screen *screen = 0;
static nanogui::Widget *mainWindow = 0;

// ref http://stackoverflow.com/questions/7302996/changing-the-delimiter-for-cin-c

struct comma_is_space : std::ctype<char> {
	comma_is_space() : std::ctype<char>(get_table()) {}

	static mask const* get_table() {
		static mask rc[table_size];
		rc[','] = std::ctype_base::space;
		return &rc[0];
	}
};


// WindowStagger - provides an automatic position for the next window
class WindowStagger {
	public:
		WindowStagger(const nanogui::Screen *s) : screen(s), next_pos(20,40), stagger(20,20) {}
		WindowStagger(const nanogui::Vector2i &stag) : next_pos(40,20), stagger(stag) {}
		
		nanogui::Vector2i pos() {
			nanogui::Vector2i pos(next_pos);
			next_pos += stagger;
			if (next_pos.x() > screen->size().x() - 100)
				next_pos.x() = 20;
			if (next_pos.y() > screen->size().y() - 100)
				next_pos.y() = 40;
			return pos;
		}
		
	private:
		const nanogui::Screen *screen;
		nanogui::Vector2i next_pos;
		nanogui::Vector2i stagger;	
};


class ClockworkExample : public nanogui::Screen {

public:
	ClockworkExample() : nanogui::Screen(nanogui::Vector2i(1024, 768), "NanoGUI Test", true, false),
			window(0), subscription_manager(0), disconnect_responder(0), connect_responder(0),
			iosh_cmd(0), cmd_interface(0), next_device_num(0), next_state_num(0),scale(1000),
			window_stagger(this) {

		using namespace nanogui;

		window = new Window(this, "");
		window->setPosition(window_stagger.pos());
		window->setFixedSize(Vector2i(400, 120));

		Widget *container = window;
		window->setLayout(new ManualLayout(size()));

		
		Label *l = new Label(container, "Label", "sans-bold");
		
		Button *b = new Button(container, "OK");
		b->setBackgroundColor(Color(200, 200, 200, 255));
		b->setFixedSize(Vector2i(60, 25));

		mainWindow = window;
		performLayout(mNVGContext);

		screen = this;				
	}

	virtual bool keyboardEvent(int key, int scancode, int action, int modifiers) override {

		if (Screen::keyboardEvent(key, scancode, action, modifiers))
			return true;

		if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
			setVisible(false);
			return true;
		}

		return false;
	}

	virtual void draw(NVGcontext *ctx) override {
		/* Draw the user interface */
		Screen::draw(ctx);
		
	}

	virtual bool mouseButtonEvent(const nanogui::Vector2i &p, int button, bool down, int modifiers) override {

		using namespace nanogui;

		if (!window->contains(p - window->position()))
			return Screen::mouseButtonEvent(p, button, down, modifiers);

		return Screen::mouseButtonEvent(p, button, down, modifiers);
	}

	virtual void drawContents() override {
		{
			
			using namespace nanogui;

			boost::mutex::scoped_lock lock(update_mutex);

			if (program_state == s_initialising) {

				std::cout << "-------- Starting Command Interface ---------\n" << std::flush;
				std::cout << "connecting to clockwork on " << host << ":" << cw_out << "\n";
				g_iodcmd = MessagingInterface::create(host, cw_out);
				g_iodcmd->start();

				iosh_cmd = new zmq::socket_t(*MessagingInterface::getContext(), ZMQ_REP);
				iosh_cmd->bind(local_commands);
				usleep(100);

				subscription_manager = new SubscriptionManager("PANEL_CHANNEL", eCLOCKWORK, "localhost", 5555);
				subscription_manager->configureSetupConnection(host.c_str(), cw_out);
				disconnect_responder = new SetupDisconnectMonitor;
				connect_responder = new SetupConnectMonitor;
				subscription_manager->monit_setup->addResponder(ZMQ_EVENT_DISCONNECTED, disconnect_responder);
				subscription_manager->monit_setup->addResponder(ZMQ_EVENT_CONNECTED, connect_responder);
				subscription_manager->setupConnections();
				program_state = s_running;
			}
			else if (program_state == s_running) {
				if (!cmd_interface) {
					cmd_interface = new zmq::socket_t(*MessagingInterface::getContext(), ZMQ_REQ);
					cmd_interface->connect(local_commands);
				}

				static bool reported_error = false;

				zmq::pollitem_t items[] = {
					{ subscription_manager->setup(), 0, ZMQ_POLLIN, 0 },
					{ subscription_manager->subscriber(), 0, ZMQ_POLLIN, 0 },
					{ *iosh_cmd, 0, ZMQ_POLLIN, 0 }
				};

				try {
					{
						int item = 1;

						try {
							int sock_type = 0;
							size_t param_size = sizeof(sock_type);
							subscription_manager->setup().getsockopt(ZMQ_TYPE, &sock_type, &param_size);
							++item;
							subscription_manager->subscriber().getsockopt(ZMQ_TYPE, &sock_type, &param_size);
							++item;
							iosh_cmd->getsockopt(ZMQ_TYPE, &sock_type, &param_size);

						}
						catch (zmq::error_t zex) {
							std::cerr << "zmq exception " << zmq_errno()  << " " << zmq_strerror(zmq_errno())
							          << " for item " << item << " in SubscriptionManager::checkConnections\n";

						}
					}

					if (!subscription_manager->checkConnections(items, 3, *iosh_cmd)) {
						if (debug) {
							std::cout << "no connection to iod\n";
							reported_error = true;
						}
					}
					else {
						zmq::message_t update;

						if (!subscription_manager->subscriber().recv(&update, ZMQ_DONTWAIT)) {
							if (errno == EAGAIN) {
								usleep(50);
							}
							else if (errno == EFSM) exit(1);
							else if (errno == ENOTSOCK) exit(1);
							else if (errno == ETIMEDOUT) {} else std::cerr << "subscriber recv: " << zmq_strerror(zmq_errno()) << "\n";
						}
						else {

							struct timeval now;
							gettimeofday(&now, 0);
							std::ostream &output(std::cout);
							long len = update.size();
							char *data = (char *)malloc(len+1);
							memcpy(data, update.data(), len);
							data[len] = 0;

							if (DEBUG_BASIC) std::cout << "received: "<<data<<" from clockwork\n";

							std::list<Value> *message = 0;

							std::string machine, property, op, state;

							Value val(SymbolTable::Null);

							if (MessageEncoding::getCommand(data, op, &message)) {
								if (op == "STATE"&& message->size() == 2) {
									machine = message->front().asString();
									message->pop_front();
									state = message->front().asString();
									int state_num = lookupState(state);
									std::map<std::string, int>::iterator idx = device_map.find(machine);

									if (idx == device_map.end()) //new machine
										device_map[machine] = next_device_num++;

									output << get_diff_in_microsecs(&now, &start)/scale
									<< "\t" << machine << "\t" << state << "\t" << state_num;
								}
								else if (op == "UPDATE") {

									//output << get_diff_in_microsecs(&now, &start)/scale << data << "\n";
									output << get_diff_in_microsecs(&now, &start)/scale;
									std::list<Value>::iterator iter = message->begin();

									while (iter!= message->end()) {
										const Value &v =  *iter++;
										output << v;

										if (iter != message->end()) output << "\t";
									}
								}
								else if (op == "PROPERTY" && message->size() == 3) {
									std::string machine = message->front().asString();
									message->pop_front();
									std::string prop = message->front().asString();
									message->pop_front();
									property = machine + "." + prop;
									val = message->front();

									std::map<std::string, int>::iterator idx = device_map.find(property);

									if (idx == device_map.end()) //new machine
										device_map[property] = next_device_num++;

									output << get_diff_in_microsecs(&now, &start)/ scale
									<< "\t" << property << "\tvalue\t";

									if (val.kind == Value::t_string)
										output << "\"" << escapeNonprintables(val.asString().c_str()) << "\"";
									else
										output << escapeNonprintables(val.asString().c_str());
								}
							}

							free(data);

							output << "\n";
						}
					}
				}
				catch (zmq::error_t zex) {
					if (zmq_errno() != EINTR) {
						std::cerr << "zmq exception " << zmq_errno()  << " "
						          << zmq_strerror(zmq_errno()) << " polling connections\n";
					}
				}
				catch (std::exception ex) {
					std::cerr << "polling connections: " << ex.what() << "\n";
				}
			}
		}
	}

	int lookupState(std::string &state) {
		int state_num = 0;
		std::map<std::string, int>::iterator idx = state_map.find(state);

		if (idx == state_map.end()) { // new state
			state_num = next_state_num;
			state_map[state] = next_state_num++;
		}
		else
			state_num = (*idx).second;

		return state_num;
	}

	std::string escapeNonprintables(const char *buf) {
		const char *hex = "0123456789ABCDEF";
		std::string res;

		while (*buf) {
			if (isprint(*buf)) res += *buf;
			else if (*buf == '\015') res += "\\r";
			else if (*buf == '\012') res += "\\n";
			else if (*buf == '\010') res += "\\t";
			else {
				const char tmp[3] = { hex[ (*buf & 0xf0) >> 4], hex[ (*buf & 0x0f)], 0 };
				res = res + "#{" + tmp + "}";
			}

			++buf;
		}

		return res;
	}

	void queueMessage(std::string s) {
		messages.push_back(s);
	}

	void queueMessage(const char *s) {
		messages.push_back(s);
	}

	char *sendIOD(int group, int addr, int new_value);
	char *sendIODMessage(const std::string &s);
	std::string getIODSyncCommand(int group, int addr, bool which);
	std::string getIODSyncCommand(int group, int addr, int new_value);
	std::string getIODSyncCommand(int group, int addr, unsigned int new_value);

protected:
	nanogui::Window *window;
	SubscriptionManager *subscription_manager;
	SetupDisconnectMonitor *disconnect_responder;
	SetupConnectMonitor *connect_responder;
	zmq::socket_t *iosh_cmd;
	zmq::socket_t *cmd_interface;
	std::list<std::string> messages; // outgoing messages
	nanogui::ref<nanogui::Window> property_window;

	MessagingInterface *g_iodcmd;

	std::map<std::string, int> state_map;
	std::map<std::string, int> device_map;

	int next_device_num;
	int next_state_num;

	struct timeval start;
	long scale;
	WindowStagger window_stagger;
};


static void finish(int sig) {

	struct sigaction sa;
	sa.sa_handler = SIG_DFL;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	sigaction(SIGTERM, &sa, 0);
	sigaction(SIGINT, &sa, 0);
	sigaction(SIGUSR1, &sa, 0);
	sigaction(SIGUSR2, &sa, 0);
	program_state = s_finished;
}

static void toggle_debug(int sig) {
	if (debug && debug != saved_debug) saved_debug = debug;

	if (debug) debug = 0;
	else {
		if (saved_debug==0) saved_debug = 1;

		debug = saved_debug;
	}
}

static void toggle_debug_all(int sig) {
	if (debug) debug = 0;
	else debug = DEBUG_ALL;
}

bool setup_signals() {

	struct sigaction sa;
	sa.sa_handler = finish;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;

	if (sigaction(SIGTERM, &sa, 0) || sigaction(SIGINT, &sa, 0)) {
		return false;
	}

	sa.sa_handler = toggle_debug;

	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;

	if (sigaction(SIGUSR1, &sa, 0) ) {
		return false;
	}

	sa.sa_handler = toggle_debug_all;

	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;

	if (sigaction(SIGUSR2, &sa, 0) ) {
		return false;
	}

	return true;
}

std::string ClockworkExample::getIODSyncCommand(int group, int addr, bool which) {
	int new_value = (which) ? 1 : 0;
	char *msg = MessageEncoding::encodeCommand("MODBUS", group, addr, new_value);
	sendIODMessage(msg);

	if (DEBUG_BASIC) std::cout << "IOD command: " << msg << "\n";

	std::string s(msg);

	free(msg);

	return s;
}

std::string ClockworkExample::getIODSyncCommand(int group, int addr, int new_value) {
	char *msg = MessageEncoding::encodeCommand("MODBUS", group, addr, new_value);
	sendIODMessage(msg);

	if (DEBUG_BASIC) std::cout << "IOD command: " << msg << "\n";

	std::string s(msg);

	free(msg);

	return s;
}

std::string ClockworkExample::getIODSyncCommand(int group, int addr, unsigned int new_value) {
	char *msg = MessageEncoding::encodeCommand("MODBUS", group, addr, new_value);
	sendIODMessage(msg);

	if (DEBUG_BASIC) std::cout << "IOD command: " << msg << "\n";

	std::string s(msg);

	free(msg);

	return s;
}

char *ClockworkExample::sendIOD(int group, int addr, int new_value) {
	std::string s(getIODSyncCommand(group, addr, new_value));

	if (g_iodcmd)
		return g_iodcmd->send(s.c_str());
	else {
		if (DEBUG_BASIC) std::cout << "IOD interface not ready\n";

		return strdup("IOD interface not ready\n");
	}
}

char *ClockworkExample::sendIODMessage(const std::string &s) {
	std::cout << "sending " << s << "\n";

	if (g_iodcmd)
		return g_iodcmd->send(s.c_str());
	else {
		if (DEBUG_BASIC) std::cout << "IOD interface not ready\n";

		return strdup("IOD interface not ready\n");
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

	program_state = s_initialising;
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

	gettimeofday(&start, 0);

	try {
		nanogui::init();

		{
			nanogui::ref<ClockworkExample> app = new ClockworkExample();
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



