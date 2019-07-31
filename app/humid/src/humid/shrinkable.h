//
//  Shrinkable.h
//  Project: Humid
//
//	All rights reserved. Use of this source code is governed by the
//	3-clause BSD License in LICENSE.txt.

#ifndef __Shrinkable_h__
#define __Shrinkable_h__

#include <ostream>
#include <string>
#include <list>
#include <nanogui/widget.h>

class Shrinkable {
public:
  Shrinkable() : shrunk_pos(nanogui::Vector2i(0,0)), shrunk(false) {}
  virtual ~Shrinkable() {}
	nanogui::Vector2i shrunkPos() { return shrunk_pos; }
	void setShrunkPos( const nanogui::Vector2i &sp ) { shrunk_pos = sp; }
	bool isShrunk() { return shrunk; }

	virtual void toggleShrunk() = 0;

protected:
	nanogui::Vector2i saved_size;
	nanogui::Vector2i saved_pos;
	nanogui::Vector2i shrunk_pos;
	bool shrunk;
};

#endif
