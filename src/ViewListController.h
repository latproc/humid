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
	bool visible;
};

class ViewListController {
public:
	void set(nanogui::Widget *w, bool vis) {
		items[w].visible = vis;
		ViewOptions vo = items[w];
		int x =1;
	}
	ViewOptions get(nanogui::Widget *w) {
		// default is for views to be visible
		if (items.find(w) == items.end())
			set(w, true);
		return items[w];
	}
	void remove(nanogui::Widget *w) { items.erase(w); }
    std::ostream &operator<<(std::ostream &out) const;

private:
	std::map<nanogui::Widget*, ViewOptions> items;
};

std::ostream &operator<<(std::ostream &out, const ViewListController &m);

#endif
