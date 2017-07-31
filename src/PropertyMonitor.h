/*
	All rights reserved. Use of this source code is governed by the
	3-clause BSD License in LICENSE.txt.
*/
#pragma once

#include <string>
#include <nanogui/common.h>
#include <nanogui/widget.h>

#include "DragHandle.h"

/*
Handles have a mode and a position. Each object has 9 handles but since only one object's handles are
active at a time we can use a singleton ObjectHandles to hold the handles and initialise it based on
the position and size of an object.

*/
using Eigen::Vector2i;

class Handle {
public:

enum Mode { NONE, POSITION, RESIZE_TL, RESIZE_T, RESIZE_TR, RESIZE_R, RESIZE_BL, RESIZE_L, RESIZE_BR, RESIZE_B};


static Handle create(Mode which, nanogui::Vector2i pos, nanogui::Vector2i size);

std::ostream &operator<<(std::ostream& out) const{ 
	return out;
}

Handle() : mMode(NONE) {}

void setPosition(Vector2i newpos) { pos = newpos; }
Vector2i position() const { return pos; }

void setMode(Mode newmode) { mMode = newmode; }
Mode mode() { return mMode; }

Handle closest(Vector2i pt) {
	Handle result;
	
	
	return result;
}

protected:
	Mode mMode;
	Vector2i pos;
};

std::ostream &operator<<(std::ostream& out, Handle::Mode m);

class PropertyMonitor {
public:
	//enum Mode { POSITION, RESIZE_TL, RESIZE_TR, RESIZE_BL, RESIZE_BR};
	PropertyMonitor() : mMode(Handle::POSITION) {}
	virtual ~PropertyMonitor() {}
	virtual void update(nanogui::DragHandle *) = 0;
	void setMode(Handle::Mode m) { mMode = m; }
	Handle::Mode mode() const { return mMode; }
protected:
	Handle::Mode mMode;	
};

class DummyMonitor : public PropertyMonitor {
public:
		virtual void update(nanogui::DragHandle *dh) override {}
};

class PositionMonitor : public PropertyMonitor {
public:
	virtual void update(nanogui::DragHandle *dh) override;
};

