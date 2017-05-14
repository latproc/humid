/*
	All rights reserved. Use of this source code is governed by the
	3-clause BSD License in LICENSE.txt.
*/
#ifndef __SKELETON_H__
#define __SKELETON_H__

#include <nanogui/screen.h>
#include <nanogui/window.h>
#include <nanogui/widget.h>
#include <MessageEncoding.h>
#include <MessagingInterface.h>
#include <signal.h>
#include <SocketMonitor.h>
#include <ConnectionManager.h>
#include "PanelScreen.h"

enum ProgramState { s_initialising, s_running, s_disconnecting, s_idle, s_finished };

class SkeletonWindow : public nanogui::Window, public PanelScreen {
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
private:
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
	const nanogui::Screen *screen;
	nanogui::Vector2i next_pos;
	nanogui::Vector2i stagger;
};

class SetupDisconnectMonitor;
class SetupConnectMonitor;

class ClockworkClient : public nanogui::Screen {

public:
	ClockworkClient();

	virtual bool keyboardEvent(int key, int scancode, int action, int modifiers) override;

	virtual void draw(NVGcontext *ctx) override;

	virtual bool mouseButtonEvent(const nanogui::Vector2i &p, int button, bool down, int modifiers) override;

	virtual void drawAll() override;
	virtual void idle(); // this routine is called after event processing and while idle

	int lookupState(std::string &state);

	std::string escapeNonprintables(const char *buf);

	void queueMessage(std::string s);

	void queueMessage(const char *s);

	char *sendIOD(int group, int addr, int new_value);
	char *sendIODMessage(const std::string &s);
	std::string getIODSyncCommand(int group, int addr, bool which);
	std::string getIODSyncCommand(int group, int addr, int new_value);
	std::string getIODSyncCommand(int group, int addr, unsigned int new_value);

	virtual void handleRawMessage(unsigned long time, void *data) {};
	virtual void handleClockworkMessage(unsigned long time, const std::string &op, std::list<Value> *message) {};
	virtual void update();

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
	uint64_t first_message_time;
	long scale;
	WindowStagger window_stagger;
};


#endif
