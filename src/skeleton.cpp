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
#include "resourcemanager.h"
#include "editorsettings.h"

long collect_history = 0;
extern Structure *system_settings;

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
	EditorSettings::setDirty();
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
		this->window->setVisible(false);
	});
	ok_button->setEnabled(true);
}


const int DEBUG_ALL = 255;
#define DEBUG_BASIC ( 1 & debug)
int debug = DEBUG_ALL;
int saved_debug = 0;

/* Clockwork Interface */
ProgramState program_state = s_initialising;
boost::mutex update_mutex;

struct timeval start;

class SetupDisconnectMonitor : public EventResponder {

public:
	void operator()(const zmq_event_t &event_, const char* addr_) {
	}
};

class SetupConnectMonitor : public EventResponder {

public:
  SetupConnectMonitor(ClockworkClient::Connection *c) : connection(c) {}
	void operator()(const zmq_event_t &event_, const char* addr_) {
    connection->setNeedsRefresh(true);
	}
private:
  ClockworkClient::Connection *connection;
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
WindowStagger::WindowStagger(const nanogui::Screen *s) : screen_widget(s), next_pos(20,40), stagger(20,20) {}
WindowStagger::WindowStagger(const nanogui::Vector2i &stag) : next_pos(40,20), stagger(stag) {}

nanogui::Vector2i WindowStagger::pos() {
		nanogui::Vector2i pos(next_pos);
		next_pos += stagger;
		if (next_pos.x() > screen_widget->size().x() - 100)
			next_pos.x() = 20;
		if (next_pos.y() > screen_widget->size().y() - 100)
			next_pos.y() = 40;
		return pos;
}

ClockworkClient::ClockworkClient(const Vector2i &size, const std::string &caption, bool resizeable, bool fullscreen)
: nanogui::Screen(size, caption, resizeable, fullscreen),
	window(0),
	window_stagger(this) {
		gettimeofday(&start, 0);
}

bool ClockworkClient::keyboardEvent(int key, int scancode, int action, int modifiers) {

	if (Screen::keyboardEvent(key, scancode, action, modifiers))
		return true;
/*
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
		setVisible(false);
		return true;
	}
	if (key == GLFW_KEY_ENTER && action == GLFW_PRESS) {
		setVisible(false);
		return true;
	}
*/
	return false;
}

void cleanupTextureCache();

void ClockworkClient::cleanupTexture(GLuint tex) { 
	deferred_texture_cleanup.push_back(std::make_pair(tex, microsecs())); 
}

void ClockworkClient::draw(NVGcontext *ctx) {
	/* Draw the user interface */
	Screen::draw(ctx);
	uint64_t now = microsecs();
	static uint64_t last_cleanup = now;
	if (now - last_cleanup > 1000) {
		last_cleanup = now;
		cleanupTextureCache();
	}
}

bool ClockworkClient::mouseButtonEvent(const nanogui::Vector2i &p, int button, bool down, int modifiers) {

		using namespace nanogui;

		nanogui::Vector2i wp(p - window->position());

		if (window->contains(p)) {
			return window->mouseButtonEvent(p, button, down, modifiers);
		}
		else {
			return Screen::mouseButtonEvent(p, button, down, modifiers);
		}
	}

ClockworkClient::Connection::Connection(ClockworkClient *cc, const std::string connection_name, const std::string ch, std::string h, int p) 
	: startup(sINIT), owner(cc), name(connection_name), channel_name(ch), host_name(h), port(p), sm(0), disconnect_responder(0),
		iosh_cmd(0), cmd_interface(0), g_iodcmd(0), command_state(WaitingCommand), last_update(0), 
		first_message_time(0),message_time_scale(1000), local_commands("inproc://local_cmds"), needs_refresh(false) {
	local_commands += "_" + connection_name;
}

void ClockworkClient::Connection::SetupInterface() {
	std::cerr << "-------- Skeleton Starting " << name << " Interface ---------\n";
	std::cerr << "connecting to clockwork on " << host_name << ":" << port << "\n";
	g_iodcmd = MessagingInterface::create(host_name, port);
	g_iodcmd->start();
	iosh_cmd = new zmq::socket_t(*MessagingInterface::getContext(), ZMQ_REP);
	std::cerr << name << " binding internally to " << local_commands << " for cmd interface\n";
	iosh_cmd->bind(local_commands.c_str());
}

zmq::socket_t *ClockworkClient::Connection::commandInterface() {
	return cmd_interface;
}

void ClockworkClient::Connection::setupCommandInterface() {
	std::cerr << name << " connecting to " << local_commands << " cmd interface\n";
	cmd_interface = new zmq::socket_t(*MessagingInterface::getContext(), ZMQ_REQ);
	cmd_interface->connect(local_commands.c_str());
}

ClockworkClient::Connection *ClockworkClient::setupConnection(Structure *s_conn) {
	const Value &chn = s_conn->getProperties().find("channel");
	const Value &host = s_conn->getProperties().find("host");
	long port = s_conn->getIntProperty("port", 5555);
	if (chn != SymbolTable::Null && host != SymbolTable::Null) {
		Connection *conn = new Connection(this, s_conn->getName(), chn.asString(), host.asString(), port);
		conn->setDefinition(s_conn);
		conn->SetupInterface();
		usleep(1000); //TBD is this necessary? do it properly...

		SubscriptionManager *sm = new SubscriptionManager(chn.asString().c_str(),
			eCLOCKWORK, host.asString().c_str(), port);
			sm->configureSetupConnection(host.asString().c_str(), port);
		 SetupDisconnectMonitor *disconnect_responder = new SetupDisconnectMonitor;
		SetupConnectMonitor *connect_responder = new SetupConnectMonitor(conn);
		sm->monit_setup->addResponder(ZMQ_EVENT_DISCONNECTED, disconnect_responder);
		sm->monit_setup->addResponder(ZMQ_EVENT_CONNECTED, connect_responder);
		sm->setupConnections();
		conn->setResponder(connect_responder);
		conn->setSubscription(sm);
		usleep(1000);
		{
		int item = 1;

		try {
			int sock_type = 0;
			size_t param_size = sizeof(sock_type);
			sm->setup().getsockopt(ZMQ_TYPE, &sock_type, &param_size);
			++item;
			sm->subscriber().getsockopt(ZMQ_TYPE, &sock_type, &param_size);
			++item;
			conn->iosh_cmd->getsockopt(ZMQ_TYPE, &sock_type, &param_size);

		}
		catch (zmq::error_t zex) {
			std::cerr << "zmq exception " << zmq_errno()  << " " << zmq_strerror(zmq_errno())
			<< " for item " << item << " in SubscriptionManager::checkConnections\n";

		}
		}
		return conn;
	}
	else {
		std::cerr << "no channel for Clockwork connection\n";
	}
	return 0;
}

bool ClockworkClient::setupConnections(Structure *project_settings) {
	int added = 0;
	if (project_settings) {
		collect_history = project_settings->getIntProperty("project_history", 100);
		StructureClass *sc = project_settings->getStructureDefinition();
		if (sc) {
			for (auto p : sc->getLocals()) {
				Structure *s_conn = p.machine;
				if (s_conn && connections.find(s_conn->getName()) == connections.end()) {
					Connection *conn = setupConnection(s_conn);
					if (conn) {
						connections.insert(std::make_pair(s_conn->getName(), conn));
						++added;
					}
				}
			}
		}
		else {
			std::cerr << "could not find a structure class for project settings\n";
		}
	}
	else {
		std::cerr << "setupConnections passed a null project settings\n";
		return false;
	}
	return added > 0;
}

bool ClockworkClient::Connection::update() {
	if (!Ready()) return false;
	uint64_t update_time = microsecs();
	if (update_time - last_update > 10000) {
		if (update_time - last_update > 15000) last_update = update_time; else last_update += 10000;
		return true;
	}
	return false;
}

zmq::socket_t* ClockworkClient::Connection::getCommandSocket() const {
	return iosh_cmd;
}

bool ClockworkClient::Connection::handleCommand(ClockworkClient *owner) {
	if (command_state == WaitingCommand && !messages.empty()) {
		std::string msg = messages.front().first;
		std::cerr << name << " sending " << msg << "\n";
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
	return command_state == WaitingCommand && !messages.empty();
}

bool ClockworkClient::Connection::handleSubscriber() {
	MessageHeader mh;
	char *data = 0;
	size_t len = 0;
	if (!safeRecv(sm->subscriber(), &data, &len, false, 0, mh) ) {
		return false;
	}
	if (first_message_time == 0) first_message_time = mh.start_time;

	std::ostream &output(std::cerr);

	if (DEBUG_BASIC)
		std::cerr << "received: "<<data<<" from connection: " << name << "\n";

	std::list<Value> *message = 0;

	uint64_t now = microsecs();
	std::string cmd;
	bool res = false;

	if (MessageEncoding::getCommand(data, cmd, &message)) {
		if (first_message_time == 0) first_message_time = microsecs();
		if (mh.start_time == 0) mh.start_time = microsecs();
		const unsigned long t = ( mh.start_time - first_message_time ) / message_time_scale;
		owner->handleClockworkMessage(this, t, cmd, message);
		delete message;
		message = 0;
		res = true;
	}
	else {
		std::cerr << "failed to decode command " << data << "\n";
	}
	free(data);
	return res;
}

bool ClockworkClient::Connection::Ready() { 
	return sm && sm->setupStatus() == SubscriptionManager::e_done;
}

void ClockworkClient::idle() {
	using namespace nanogui;

	boost::mutex::scoped_lock lock(update_mutex);
	Structure *connection = 0;
	static uint64_t last_check_new_connections = 0;

	Structure *project_settings = findStructure("ProjectSettings");
	if (program_state == s_initialising) {
			
		//if (setupConnections(project_settings)) 
			program_state = s_running;
	}
	else if (program_state == s_running) {

		uint64_t now = microsecs();
		if (now - last_check_new_connections > 1000000) {// attempt any new connections every second
			if (setupConnections(project_settings))
				std::cerr << "--- added connection\n";
			last_check_new_connections = now;
		}

		std::map<std::string, Connection *>::iterator iter = connections.begin();
		while (iter != connections.end()) {
			const std::pair<std::string, Connection *>item = *iter++;
			{
				Connection *conn = item.second;
				if (!conn->commandInterface())
					conn->setupCommandInterface();

				static bool reported_error = false;

				SubscriptionManager *subscription_manager = conn->subscriptionManager();
				if (!subscription_manager) continue;

				{
					if (conn->Ready() && conn->update()) 
						update(conn);
					zmq::pollitem_t items[] = {
						{ subscription_manager->setup(), 0, ZMQ_POLLIN, 0 },
						{ subscription_manager->subscriber(), 0, ZMQ_POLLIN, 0 },
						{ *conn->iosh_cmd, 0, ZMQ_POLLIN, 0 }
					};

					try {
						if (!subscription_manager->checkConnections(items, 3, *conn->iosh_cmd)) {
							if (debug) {
								std::cerr << conn->getName() << ": no connection to iod\n";
								reported_error = true;
							}
						}
						else  if (subscription_manager->setupStatus() == SubscriptionManager::e_done) {
							int loop_counter = 400;
							if (conn->getStartupState() == sSTARTUP) conn->refreshData();
							conn->handleCommand(this);
							while (loop_counter--) {
								if (!conn->handleSubscriber()) break;
							}
						}
					}
					catch (zmq::error_t zex) {
						if (zmq_errno() != EINTR) {
							std::cerr << "zmq exception " << zmq_errno()  << " "
							<< zmq_strerror(zmq_errno()) << " polling connection " << conn->getName() << "\n";
						}
					}
					catch (std::exception ex) {
						std::cerr << "polling connection: " << conn->getName() << " " << ex.what() << "\n";
					}
				}
				usleep(10);
			}
		}
	}
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

void ClockworkClient::Connection::queueMessage(const std::string s, std::function< void(std::string) >f) {
	messages.push_back(std::make_pair(s, f) );
}
void ClockworkClient::Connection::queueMessage(const char *s, std::function< void(std::string) >f) {
	messages.push_back(std::make_pair(s, f) );
}

ClockworkClient::Connection *findConnection(const std::string &connection_name, std::map<std::string, ClockworkClient::Connection*> &connections) {
	auto found = connections.find(connection_name);
	if (found != connections.end()) return (*found).second;
	return 0;
}

void ClockworkClient::queueMessage(const std::string & connection_name, const std::string s, std::function< void(const std::string) >f) {
	Connection *conn = findConnection(connection_name, connections); 
	if (conn) conn->queueMessage(s, f);
}

void ClockworkClient::queueMessage(const std::string & connection_name, const char *s, std::function< void(const std::string) >f) {
	Connection *conn = findConnection(connection_name, connections);
	if (conn) conn->queueMessage(s, f);
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

std::string ClockworkClient::getIODSyncCommand(const std::string & connection_name, int group, int addr, bool which) {
	int new_value = (which) ? 1 : 0;
	char *msg = MessageEncoding::encodeCommand("MODBUS", group, addr, new_value);

	if (DEBUG_BASIC) std::cerr << "IOD command: " << msg << "\n";
	std::string s(msg);
	free(msg);
	return s;
}

std::string ClockworkClient::getIODSyncCommand(const std::string & connection_name, int group, int addr, int new_value) {
	char *msg = MessageEncoding::encodeCommand("MODBUS", group, addr, new_value);
	//conn->sendIODMessage(msg);
	if (DEBUG_BASIC) std::cerr << "IOD command: " << msg << "\n";
	std::string s(msg);
	free(msg);
	return s;
}

std::string ClockworkClient::getIODSyncCommand(const std::string & connection_name, int group, int addr, unsigned int new_value) {
	char *msg = MessageEncoding::encodeCommand("MODBUS", group, addr, new_value);
	//conn->sendIODMessage(msg);
	if (DEBUG_BASIC) std::cerr << "IOD command: " << msg << "\n";
	std::string s(msg);
	free(msg);

	return s;
}

std::string ClockworkClient::getIODSyncCommand(const std::string & connection_name, int group, int addr, float new_value) {
		char *msg = MessageEncoding::encodeCommand("MODBUS", group, addr, new_value);

		if (DEBUG_BASIC) std::cerr << "IOD command: " << msg << "\n";
		std::string s(msg);
		free(msg);
		return s;
}
std::string ClockworkClient::getIODSyncCommand(const std::string & connection_name, int group, int addr, const char *new_value) {
	char *msg = MessageEncoding::encodeCommand("MODBUS", group, addr, new_value);

		if (DEBUG_BASIC) std::cerr << "IOD command: " << msg << "\n";
		std::string s(msg);
		free(msg);
		return s;
}

char *ClockworkClient::Connection::sendIOD(const char *msg) {
	if (!Ready()) return 0;
	std::string s(msg);
	return sendIODMessage(s);
}

char *ClockworkClient::Connection::sendIODMessage(const std::string &s) {
	if (!Ready()) return 0;

	std::cerr << "sendIOD sending " << s << "\n";

	if (g_iodcmd)
		return g_iodcmd->send(s.c_str());
	else {
		if (DEBUG_BASIC) std::cerr << "IOD interface not ready\n";

		return strdup("IOD interface not ready\n");
	}
}

char *ClockworkClient::sendIOD(const std::string & connection_name, int group, int addr, int new_value) {
	Connection *conn = findConnection(connection_name, connections);
	std::string s(getIODSyncCommand(connection_name, group, addr, new_value));
	std::cerr << "sendIOD sending " << s << "\n";
	if (conn)
		return conn->sendIODMessage(s);
	return 0;
}

char *ClockworkClient::sendIODMessage(const std::string & connection_name, const std::string &s) {
	Connection *conn = findConnection(connection_name, connections);
	if (conn)
		return conn->sendIODMessage(s);
	return 0;
}

void ClockworkClient::update(ClockworkClient::Connection *) { }

void ClockworkClient::drawAll() {
	idle();
	Screen::drawAll();
}
