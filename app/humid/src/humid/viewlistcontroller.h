//
//  ViewListController.h
//  Project: humid
//
//	All rights reserved. Use of this source code is governed by the
//	3-clause BSD License in LICENSE.txt.

#ifndef __ViewListController_h__
#define __ViewListController_h__

#include <ostream>
#include <string>
#include <map>
#include <nanogui/widget.h>


class ViewOptions {
public:
	ViewOptions() : visible(false) {}
	bool visible;
};

class ViewListController {
public:
	ViewListController() {}
	void set(std::string name, bool vis) {
		items[name].visible = vis;
	}
	ViewOptions get(std::string name, bool default_visibility = true) {
		// default is for views to be visible
		auto found = items.find(name);
		if (found == items.end()) {
			set(name, default_visibility);
			return items[name];
		}
		return (*found).second;
	}
	void remove(std::string name) { items.erase(name); }
    std::ostream &operator<<(std::ostream &out) const;

private:
	std::map<std::string, ViewOptions> items;
};

std::ostream &operator<<(std::ostream &out, const ViewListController &m);

#endif
