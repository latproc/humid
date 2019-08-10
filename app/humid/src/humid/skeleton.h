/*
	All rights reserved. Use of this source code is governed by the
	3-clause BSD License in LICENSE.txt.
*/
#ifndef __SKELETON_H__
#define __SKELETON_H__

#include <nanogui/screen.h>
#include <nanogui/window.h>
#include <nanogui/widget.h>
#include <nanogui/opengl.h>
// #include <zmq.hpp>
// #include <signal.h>
#include <lib_clockwork_client.hpp>
#include "panelscreen.h"
#include "shrinkable.h"

enum ProgramState { s_initialising, s_running, s_disconnecting, s_idle, s_finished };

class Structure;

// SkeletonWindow is the base class for editor panels, property window and content area
class SkeletonWindow : public nanogui::Window, public PanelScreen, public Shrinkable {
public:
	SkeletonWindow(Widget *parent, const std::string &title = "Untitled")
	: Window(parent, title), PanelScreen(title), move_listener( [](nanogui::Window* value){ } ) {
	}
	void setMoveListener( std::function<void(nanogui::Window*)> f) {
		move_listener = f;
	}
	bool mouseDragEvent(const nanogui::Vector2i &p, const nanogui::Vector2i &rel,
						int button, int modifiers) override {
		bool res = nanogui::Window::mouseDragEvent(p, rel, button, modifiers);
		move_listener(this);
		return res;
	}
	virtual bool mouseButtonEvent(const nanogui::Vector2i &p, int button, bool down, int modifiers) override;
	void toggleShrunk() override;

protected:
	std::function<void(nanogui::Window*)> move_listener;
};

class Skeleton {
public:
	Skeleton(nanogui::Screen *screen);
	Skeleton(nanogui::Screen *screen, SkeletonWindow *);
	nanogui::Window *getWindow();
	SkeletonWindow *getSkeletonWindow() { return window; }
	//virtual bool mouseButtonEvent(const nanogui::Vector2i &p, int button, bool down, int modifiers);
protected:
	SkeletonWindow *window;
};

class ConfirmDialog : public Skeleton {
public:
	ConfirmDialog(nanogui::Screen *screen, const std::string msg);
	void setVisible(bool which) { window->setVisible(which); }
	void setCallback(const std::function<void()> &callback);
private:
	std::string message;
	nanogui::Button *ok_button;
};

// WindowStagger - provides an automatic position for the next window
class WindowStagger {
public:
	WindowStagger(const nanogui::Screen *s);
	WindowStagger(const nanogui::Vector2i &stag);

	nanogui::Vector2i pos();

private:
	const nanogui::Screen *screen_widget;
	nanogui::Vector2i next_pos;
	nanogui::Vector2i stagger;
};

class SetupDisconnectMonitor;
class SetupConnectMonitor;

class ClockworkClient : public nanogui::Screen {

public:


	enum CommandState { WaitingCommand, WaitingResponse };
	enum STARTUP_STATES { sSTARTUP, sINIT, sSENT, sDONE, sRELOAD };

	class Connection {
	public:
		Connection(ClockworkClient *, const std::string connection_name,
			const std::string ch, std::string h, int p);

		void SetupInterface();
		void setSubscription(SubscriptionManager *subs) { sm = subs; }
		void setResponder(SetupConnectMonitor *r) { connect_responder = r; }
		void setDisconnectResponder(SetupDisconnectMonitor *r) { disconnect_responder = r; }
		zmq::socket_t *commandInterface();
		void setupCommandInterface();
		void setDefinition(Structure *s) { definition = s; }
		Structure *getDefinition() { return definition; }

		bool update();
		bool handleCommand(ClockworkClient*);
		bool handleSubscriber();

		SubscriptionManager *subscriptionManager() { return sm; }
		const std::string &getName() { return name; }
		uint64_t getFirstMessageTime() { return first_message_time; }
		zmq::socket_t* getCommandSocket() const;

		std::list< std::pair< std::string, std::function<void(std::string)> > > &getMessages();
		void queueMessage(const std::string s, std::function< void(std::string) >f);
		void queueMessage(const char *s, std::function< void(std::string) >f);

		char *sendIOD(const char *msg);
		char *sendIODMessage(const std::string &s);

		void refreshData() { startup = sINIT; }
		STARTUP_STATES getStartupState() { return startup; }
		void setState(STARTUP_STATES new_state) { startup = new_state; }

		bool Ready();

	protected:
		STARTUP_STATES startup;
		ClockworkClient *owner;
		std::string name;
		std::string channel_name;
		std::string host_name;
		int port;
		Structure *definition;
		SubscriptionManager *sm;
		SetupDisconnectMonitor *disconnect_responder;
		SetupConnectMonitor *connect_responder;
	public:
		zmq::socket_t *iosh_cmd;
		zmq::socket_t *cmd_interface;
	protected:
		std::list< std::pair< std::string, std::function<void(std::string)> > > messages; // outgoing messages
		MessagingInterface *g_iodcmd;
		CommandState command_state;
		uint64_t last_update;
		uint64_t first_message_time;
		long message_time_scale;
		std::string local_commands;

	};

	ClockworkClient(const Eigen::Vector2i &size, const std::string &caption, bool resizeable = true, bool fullscreen = false);

	Connection *setupConnection(Structure *s_conn);
	bool setupConnections(Structure *project_settings);


	virtual bool keyboardEvent(int key, int scancode, int action, int modifiers) override;

	virtual void draw(NVGcontext *ctx) override;

	virtual bool mouseButtonEvent(const nanogui::Vector2i &p, int button, bool down, int modifiers) override;

	virtual void drawAll() override;
	virtual void idle(); // this routine is called after event processing and while idle

	int lookupState(std::string &state);

	std::string escapeNonprintables(const char *buf);

	void queueMessage(const std::string & connection_name, const std::string s, std::function< void(std::string) >f);

	void queueMessage(const std::string & connection_name, const char *s, std::function< void(std::string) >f);

	std::string getIODSyncCommand(const std::string & connection_name, int group, int addr, bool which);
	std::string getIODSyncCommand(const std::string & connection_name, int group, int addr, int new_value);
	std::string getIODSyncCommand(const std::string & connection_name, int group, int addr, unsigned int new_value);
	std::string getIODSyncCommand(const std::string & connection_name, int group, int addr, float new_value);
	std::string getIODSyncCommand(const std::string & connection_name, int group, int addr, const char *new_value);

	char *sendIOD(const std::string & connection_name, int group, int addr, int new_value);
	char *sendIODMessage(const std::string & connection_name, const std::string &s);

	virtual void handleRawMessage(unsigned long time, void *data) {};
	virtual void handleClockworkMessage(ClockworkClient::Connection *conn, unsigned long time, const std::string &op, std::list<Value> *message) {};
	virtual void update(ClockworkClient::Connection *connection);

	std::map<std::string, Connection *>getConnections() { return connections; }

	void cleanupTexture(GLuint tex);

protected:
	nanogui::Window *window;
	std::map<std::string, Connection *>connections;

	struct timeval start;
	nanogui::ref<nanogui::Window> property_window;
	WindowStagger window_stagger;
	std::list< std::pair<GLuint, uint64_t> > deferred_texture_cleanup;
};


#endif
