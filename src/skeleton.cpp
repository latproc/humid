/*
	All rights reserved. Use of this source code is governed by the
	3-clause BSD License in LICENSE.txt.
*/
#include <nanogui/screen.h>
#include <nanogui/window.h>
#include <nanogui/layout.h>
#include <nanogui/label.h>
#include <nanogui/button.h>
#include "skeleton.h"
#include "manuallayout.h"
#include <iostream>

#include <nanogui/glutil.h>
#include <fstream>
#include <locale>
#include <string>
#include "propertymonitor.h"
#include "draghandle.h"


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

#include "helper.h"
#include "structure.h"

long collect_history = 0;

struct ConnectionInfo {
	std::string channel_name;
	std::string host_name;
	int port;
	ConnectionInfo(std::string ch, std::string h, int p) : channel_name(ch), host_name(h), port(p) {}
};

Skeleton::Skeleton(nanogui::Screen *screen) : window(0) {
using namespace nanogui;
	window = new SkeletonWindow(screen, "");
	window->setPosition(Vector2i(40, 40));
	window->setFixedSize(Vector2i(400, 120));
	window->setCursor(nanogui::Cursor::Hand);


	//GridLayout *layout = new GridLayout(Orientation::Horizontal,1);
	//window->setLayout(layout);
	window->setLayout(new ManualLayout(window->size()));
}

Skeleton::Skeleton(nanogui::Screen *screen, SkeletonWindow *w) {
	using namespace nanogui;
	window = w;
	window->setPosition(Vector2i(40, 40));
	window->setFixedSize(Vector2i(400, 120));
	window->setCursor(nanogui::Cursor::Hand);


	//GridLayout *layout = new GridLayout(Orientation::Horizontal,1);
	//window->setLayout(layout);
	window->setLayout(new ManualLayout(screen->size()));
}

/*
bool Skeleton::mouseButtonEvent(const nanogui::Vector2i &p, int button, bool down, int modifiers) {
	for (auto w : window->children()) {
		if (w->contains(p - w->position()))
			return w->mouseButtonEvent(p - w->position(), button, down, modifiers);
	}
	return false;
}
*/

nanogui::Window *Skeleton::getWindow() { return window; }

void ConfirmDialog::setCallback(const std::function<void()> &callback) {
	ok_button->setCallback(callback);
}

nanogui::Vector2i fixPositionInWindow(const nanogui::Vector2i &pos, const nanogui::Vector2i &siz, const nanogui::Vector2i &area) {
	nanogui::Vector2i res(pos);
	if (pos.x() < 0) res.x() = 0;
	if (pos.y() < 0) res.y() = 0;
	if (pos.x() + siz.x() > area.x()) res.x() = area.x() - siz.x();
	if (pos.y() + siz.y() > area.y()) res.y() = area.y() - siz.y();
	return res;
}

bool SkeletonWindow::mouseButtonEvent(const nanogui::Vector2i &p, int button, bool down, int modifiers) {
	if (button == GLFW_MOUSE_BUTTON_RIGHT && !down) {
		if (shrunk) {
			shrunk = false;
			setFixedSize(saved_size);
			saved_pos = fixPositionInWindow(saved_pos, size(), parent()->size());
			setPosition(saved_pos);
		}
		else {
			saved_size = fixedSize(); saved_pos = position();
			setFixedSize(nanogui::Vector2i(80, 20));
			if (shrunk_pos != nanogui::Vector2i(0,0) ) setPosition(shrunk_pos);
			shrunk = true;
		}
		requestFocus();
		nanogui::Widget *p = parent();
		nanogui::Widget *w = this;
		while (p && p->parent()) {
			if (p->parent()) { w = p; p = p->parent(); }
		}
		if (w) {
			nanogui::Screen *s = dynamic_cast<nanogui::Screen *>(p);
			if (s)
				s->performLayout();
		}
	}
	return nanogui::Window::mouseButtonEvent(p, button, down, modifiers);
}


class SkeletonButton : public nanogui::Button {

public:

	SkeletonButton(Widget *parent, const std::string &caption = "Untitled", int icon = 0)
	: nanogui::Button(parent, caption, icon) {
	}

	virtual bool mouseButtonEvent(const nanogui::Vector2i &p, int button, bool down, int modifiers) override {
		nanogui::Button *btn = dynamic_cast<nanogui::Button*>(this);
		using namespace nanogui;
		std::cout << caption() << " button" << "\n";
		return Button::mouseButtonEvent(p, button, down, modifiers);
	}

	virtual bool mouseMotionEvent(const nanogui::Vector2i &p, const nanogui::Vector2i &rel, int button, int modifiers) override {

		return nanogui::Button::mouseMotionEvent(p, rel, button, modifiers);

		return true;
	}

	virtual bool mouseEnterEvent(const nanogui::Vector2i &p, bool enter) override {

		return nanogui::Button::mouseEnterEvent(p, enter);
	}
};


ConfirmDialog::ConfirmDialog(nanogui::Screen *screen, std::string msg) : Skeleton(screen), message(msg) {
	using namespace nanogui;
	Label *itemText = new Label(window, "", "sans-bold");
	itemText->setPosition(Vector2i(40, 40));
	itemText->setSize(Vector2i(260, 20));
	itemText->setFixedSize(Vector2i(260, 20));
	itemText->setCaption(message.c_str());

	ok_button = new Button(window, "OK");
	ok_button->setPosition(Vector2i(160, 80));
	ok_button->setFixedSize(Vector2i(60, 20));
	ok_button->setSize(Vector2i(60, 20));
	ok_button->setBackgroundColor(Color(140, 140, 140, 255));
	ok_button->setCallback([this] {
		std::cout << "test\n";
		this->window->setVisible(false);
	});
	ok_button->setEnabled(true);
}


const int DEBUG_ALL = 255;
#define DEBUG_BASIC ( 1 & debug)
static nanogui::DragHandle *drag_handle = 0;
int debug = DEBUG_ALL;
int saved_debug = 0;

/* Clockwork Interface */
ProgramState program_state = s_initialising;
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
		rc[(unsigned int)','] = std::ctype_base::space;
		return &rc[0];
	}
};


// WindowStagger - provides an automatic position for the next window
WindowStagger::WindowStagger(const nanogui::Screen *s) : screen(s), next_pos(20,40), stagger(20,20) {}
WindowStagger::WindowStagger(const nanogui::Vector2i &stag) : next_pos(40,20), stagger(stag) {}

nanogui::Vector2i WindowStagger::pos() {
		nanogui::Vector2i pos(next_pos);
		next_pos += stagger;
		if (next_pos.x() > screen->size().x() - 100)
			next_pos.x() = 20;
		if (next_pos.y() > screen->size().y() - 100)
			next_pos.y() = 40;
		return pos;
}

ClockworkClient::ClockworkClient(const Vector2i &size, const std::string &caption)
: nanogui::Screen(size, caption, true, false),
	window(0), subscription_manager(0), disconnect_responder(0), connect_responder(0),
	iosh_cmd(0), cmd_interface(0), command_state(WaitingCommand), next_device_num(0), next_state_num(0),
	first_message_time(0), scale(1000),
	window_stagger(this) {
		screen = this;
		gettimeofday(&start, 0);
}

bool ClockworkClient::keyboardEvent(int key, int scancode, int action, int modifiers) {

	if (Screen::keyboardEvent(key, scancode, action, modifiers))
		return true;

	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
		setVisible(false);
		return true;
	}
	if (key == GLFW_KEY_ENTER && action == GLFW_PRESS) {
		setVisible(false);
		return true;
	}

	return false;
}

void ClockworkClient::draw(NVGcontext *ctx) {
	/* Draw the user interface */
	Screen::draw(ctx);
}

bool ClockworkClient::mouseButtonEvent(const nanogui::Vector2i &p, int button, bool down, int modifiers) {

		using namespace nanogui;

		nanogui::Vector2i wp(p - window->position());

		if (window->contains(p)) {
			std::cout << "w " << wp.x() << "," << wp.y() << " " << button << " " << down << "\n";
			return window->mouseButtonEvent(p, button, down, modifiers);
		}
		else {
			std::cout << " " << p.x() << "," << p.y() << " " << button << " " << down << "\n";
			//	return window->mouseButtonEvent(wp, button, down, modifiers);

			return Screen::mouseButtonEvent(p, button, down, modifiers);
		}
	}

void ClockworkClient::idle() {
	{
		using namespace nanogui;
		static uint64_t last_update = 0;

		boost::mutex::scoped_lock lock(update_mutex);
		Structure *connection = 0;

		if (program_state == s_initialising) {

			std::cout << "-------- Skeleton Starting Command Interface ---------\n" << std::flush;
			std::cout << "connecting to clockwork on " << host << ":" << cw_out << "\n";
			g_iodcmd = MessagingInterface::create(host, cw_out);
			g_iodcmd->start();

			iosh_cmd = new zmq::socket_t(*MessagingInterface::getContext(), ZMQ_REP);
			iosh_cmd->bind(local_commands);
			usleep(1000);

			subscription_manager = 0;
			Structure *project_settings = findStructure("ProjectSettings");
			if (project_settings ) {
				Structure *project_settings = findStructure("ProjectSettings");
				if (project_settings) {
					const Value &collect_history_v(project_settings->getProperties().find("project_history"));
					if (collect_history_v != SymbolTable::Null) {
						collect_history_v.asInteger(collect_history);
					}
				}
				StructureClass *sc = project_settings->getStructureDefinition();
				if (sc) {
					for (auto p : sc->getLocals()) {
						Structure *conn = p.machine;
						assert(conn);
						if (conn) {
								const Value &chn = conn->getProperties().find("channel");
								const Value &host = conn->getProperties().find("host");
								const Value &port = conn->getProperties().find("port");
								std::cout << "Loaded connection details " << chn << " " << host << " " << port << "\n";
								if (chn != SymbolTable::Null && host != SymbolTable::Null && port != SymbolTable::Null) {
									subscription_manager = new SubscriptionManager(chn.asString().c_str(),
										eCLOCKWORK, host.asString().c_str(), std::atoi(port.asString().c_str()));
										subscription_manager->configureSetupConnection(host.asString().c_str(), cw_out);
										disconnect_responder = new SetupDisconnectMonitor;
										connect_responder = new SetupConnectMonitor;
										subscription_manager->monit_setup->addResponder(ZMQ_EVENT_DISCONNECTED, disconnect_responder);
										subscription_manager->monit_setup->addResponder(ZMQ_EVENT_CONNECTED, connect_responder);
										subscription_manager->setupConnections();
								}
							assert(conn);
							connections.push_back(std::make_pair(conn, subscription_manager));
						}
					}
				}
			}
			if (!subscription_manager) {
				std::cout << "Warning: using default clockwork connection to localhost\n";
				subscription_manager = new SubscriptionManager("PANEL_CHANNEL", eCLOCKWORK, host.c_str(), 5555);
				subscription_manager->configureSetupConnection(host.c_str(), cw_out);
				disconnect_responder = new SetupDisconnectMonitor;
				connect_responder = new SetupConnectMonitor;
				subscription_manager->monit_setup->addResponder(ZMQ_EVENT_DISCONNECTED, disconnect_responder);
				subscription_manager->monit_setup->addResponder(ZMQ_EVENT_CONNECTED, connect_responder);
				subscription_manager->setupConnections();
				//ConnectionInfo *conn = new ConnectionInfo("PANEL_CHANNEL", host.c_str(), 5555);
				Structure *s = new Structure(0, "Default", "CONNECTION");
				connections.push_back(std::make_pair(s, subscription_manager));
			}
			program_state = s_running;
		}
		else if (program_state == s_running) {
			if (connections.size()) {
				connection = connections.front().first;
				subscription_manager = connections.front().second;
				assert(connection);
			}
			uint64_t update_time = microsecs();
			if (update_time - last_update > 10000) {
				if (update_time - last_update > 15000) last_update = update_time; else last_update += 10000;
				update(connection);
			}
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

				int loop_counter = 80;

				if (!subscription_manager->checkConnections(items, 3, *iosh_cmd)) {
					if (debug) {
						std::cout << "no connection to iod\n";
						reported_error = true;
					}
				}
				else while (loop_counter--) {
					if (command_state == WaitingCommand && !messages.empty()) {
						std::string msg = messages.front().first;
						safeSend(*cmd_interface, msg.c_str(), msg.length());
						command_state = WaitingResponse;
					}
					else if (command_state == WaitingResponse) {
						char *buf = 0;
						size_t len = 0;
						if (safeRecv(*cmd_interface, &buf, &len, false, 0)) {
							std::string s = messages.front().first;
							if (buf) {
								buf[len] = 0;
								messages.front().second(buf);
							}
							messages.pop_front();
							delete[] buf;
							command_state = WaitingCommand;
						}
					}

					MessageHeader mh;
					char *data = 0;
					size_t len = 0;
					static int msg_count = 0;
					if (!safeRecv(subscription_manager->subscriber(), &data, &len, false, 0, mh) ) {
						return;
					}
					if (msg_count++ < 10)
						std::cout << "Message header start time: " << mh.start_time << "\n";
					if (first_message_time == 0) first_message_time = mh.start_time;

					{
						std::ostream &output(std::cout);

						if (DEBUG_BASIC)
							std::cout << "received: "<<data<<" from clockwork\n";

						std::list<Value> *message = 0;

						std::string machine, property, op, state;

						Value val(SymbolTable::Null);
						uint64_t now = microsecs();

						if (MessageEncoding::getCommand(data, op, &message)) {
							//assert(mh.start_time != 0);
							//assert(first_message_time != 0);
							if (first_message_time == 0) first_message_time = microsecs();
							if (mh.start_time == 0) mh.start_time = microsecs();
							const unsigned long t = ( mh.start_time - first_message_time)/scale;
							handleClockworkMessage(t, op, message);
						}
						free(data);
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

int ClockworkClient::lookupState(std::string &state) {
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

std::string ClockworkClient::escapeNonprintables(const char *buf) {
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

void ClockworkClient::queueMessage(const std::string s, std::function< void(const std::string) >f) {
	messages.push_back(std::make_pair(s, f) );
}

void ClockworkClient::queueMessage(const char *s, std::function< void(const std::string) >f) {
	messages.push_back(std::make_pair(s, f) );
}

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

std::string ClockworkClient::getIODSyncCommand(int group, int addr, bool which) {
	int new_value = (which) ? 1 : 0;
	char *msg = MessageEncoding::encodeCommand("MODBUS", group, addr, new_value);
	sendIODMessage(msg);

	if (DEBUG_BASIC) std::cout << "IOD command: " << msg << "\n";

	std::string s(msg);

	free(msg);

	return s;
}

std::string ClockworkClient::getIODSyncCommand(int group, int addr, int new_value) {
	char *msg = MessageEncoding::encodeCommand("MODBUS", group, addr, new_value);
	sendIODMessage(msg);

	if (DEBUG_BASIC) std::cout << "IOD command: " << msg << "\n";

	std::string s(msg);

	free(msg);

	return s;
}

std::string ClockworkClient::getIODSyncCommand(int group, int addr, unsigned int new_value) {
	char *msg = MessageEncoding::encodeCommand("MODBUS", group, addr, new_value);
	sendIODMessage(msg);

	if (DEBUG_BASIC) std::cout << "IOD command: " << msg << "\n";

	std::string s(msg);

	free(msg);

	return s;
}

std::string ClockworkClient::getIODSyncCommand(int group, int addr, float new_value) {
	char *msg = MessageEncoding::encodeCommand("MODBUS", group, addr, new_value);
	sendIODMessage(msg);

	if (DEBUG_BASIC) std::cout << "IOD command: " << msg << "\n";

	std::string s(msg);

	free(msg);

	return s;
}
std::string ClockworkClient::getIODSyncCommand(int group, int addr, const char *new_value) {
	char *msg = MessageEncoding::encodeCommand("MODBUS", group, addr, new_value);
	sendIODMessage(msg);

	if (DEBUG_BASIC) std::cout << "IOD command: " << msg << "\n";

	std::string s(msg);

	free(msg);

	return s;
}
char *ClockworkClient::sendIOD(int group, int addr, int new_value) {
	std::string s(getIODSyncCommand(group, addr, new_value));
	std::cout << "sendIOD sending " << s << "\n";

	if (g_iodcmd)
		return g_iodcmd->send(s.c_str());
	else {
		if (DEBUG_BASIC) std::cout << "IOD interface not ready\n";

		return strdup("IOD interface not ready\n");
	}
}

char *ClockworkClient::sendIODMessage(const std::string &s) {
	std::cout << "sendIODMessage sending " << s << "\n";

	if (g_iodcmd)
		return g_iodcmd->send(s.c_str());
	else {
		if (DEBUG_BASIC) std::cout << "IOD interface not ready\n";

		return strdup("IOD interface not ready\n");
	}
}

void ClockworkClient::update(Structure *) { }

void ClockworkClient::drawAll() {
	idle();
	Screen::drawAll();
}
