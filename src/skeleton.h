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
#include <MessageEncoding.h>
#include <MessagingInterface.h>
#include <signal.h>
#include <zmq.hpp>
#include <SocketMonitor.h>
#include <ConnectionManager.h>
#include "panelscreen.h"

enum ProgramState { s_initialising, s_running, s_disconnecting, s_idle, s_finished };

class Structure;
class SkeletonWindow : public nanogui::Window, public PanelScreen {
public:
	SkeletonWindow(Widget *parent, const std::string &title = "Untitled")
	: Window(parent, title), PanelScreen(title), move_listener( [](nanogui::Window* value){ } ),
		shrunk_pos(nanogui::Vector2i(0,0)), shrunk(false) {
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
	nanogui::Vector2i shrunkPos() { return shrunk_pos; }
	void setShrunkPos( const nanogui::Vector2i &sp ) { shrunk_pos = sp; }
	bool isShrunk() { return shrunk; }
protected:
	std::function<void(nanogui::Window*)> move_listener;
	nanogui::Vector2i saved_size;
	nanogui::Vector2i saved_pos;
	nanogui::Vector2i shrunk_pos;

	bool shrunk;
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
		void setNeedsRefresh(bool which) { needs_refresh = which; }
		bool needsRefresh() { return needs_refresh; }

		// True once SubscriptionManager has been e_done in a prior idle pass.
		// Used to detect loss and recovery even when ZMQ setup CONNECTED is missed
		// (e.g. SUB/CHANNEL recovery while setup TCP stays up after iod restart).
		bool channelWasReady() const { return channel_was_ready; }
		void setChannelWasReady(bool which) { channel_was_ready = which; }

		// Peer link lifecycle (iod restart / network blip)
		void noteDisconnected(const char *addr = nullptr);
		void noteConnected(const char *addr = nullptr);
		void onChannelLost(const char *addr = nullptr);
		void onChannelBecameReady();
		void resetCommandPath();

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
		// Time the local REQ/REP leg as well as SubscriptionManager's remote
		// request.  A reconnect-era inproc send can be lost before the remote
		// timeout is armed, otherwise leaving Humid in WaitingResponse forever.
		uint64_t command_request_start;
		uint64_t last_update;
		uint64_t first_message_time;
		long message_time_scale;
		std::string local_commands;
		bool needs_refresh;
		bool channel_was_ready;

	};

	ClockworkClient(const nanogui::Vector2i &size, const std::string &caption, bool resizeable = true, bool fullscreen = false);

	Connection *setupConnection(Structure *s_conn);
	bool setupConnections(Structure *project_settings);
	

	virtual bool keyboardEvent(int key, int scancode, int action, int modifiers) override;
	virtual bool resizeEvent(const nanogui::Vector2i &size) override;
	virtual bool focusEvent(bool focused) override;
	void selectPreferredMonitor(bool force = false);
	// Re-layout fullscreen and present for a short window after a display
	// comes back (HDMI HPD, DPMS, or GLFW monitor connect). GLFW RandR can
	// be marked broken when every output is disconnected, so DRM scanout
	// state is the compositor-independent source of truth.
	void noteDisplayRestored();

	virtual void draw(NVGcontext *ctx) override;

	virtual bool mouseButtonEvent(const nanogui::Vector2i &p, int button, bool down, int modifiers) override;
	virtual bool mouseMotionEvent(const nanogui::Vector2i &p, const nanogui::Vector2i &rel, int button, int modifiers) override;
	virtual bool scrollEvent(const nanogui::Vector2i &p, const nanogui::Vector2f &rel) override;

	virtual void drawAll() override;
	virtual void idle(bool gui_is_ready = true); // this routine is called after event processing and while idle

	// Request a full GPU frame. The main loop still runs idle() every wake to
	// service Clockwork, but skips clear/draw/swap when nothing needs painting.
	void requestRedraw() { needs_frame_redraw = true; }
	bool needsRedraw() const { return needs_frame_redraw; }

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
	virtual void update(ClockworkClient::Connection *connection, bool allow_data_sync);
	virtual void afterFrameRendered() {}

	std::map<std::string, Connection *>getConnections() { return connections; }

	void cleanupTexture(GLuint tex);

protected:
	nanogui::Window *window;
	std::map<std::string, Connection *>connections;

	struct timeval start;
	nanogui::ref<nanogui::Window> property_window;
	WindowStagger window_stagger;
	std::list< std::pair<GLuint, uint64_t> > deferred_texture_cleanup;
	bool needs_frame_redraw;
	bool drm_watch_ready = false;
	bool drm_output_active = false;
	bool drm_last_raw_active = false;
	uint64_t drm_raw_changed_at = 0;
	uint64_t display_restore_until = 0;

	void pollDisplayOutputs();
	void rebindDisplay();
};


#endif
