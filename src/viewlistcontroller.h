//
//  ViewListController.h
//  Project: humid
//
//	All rights reserved. Use of this source code is governed by the
//	3-clause BSD License in LICENSE.txt.

#ifndef __ViewListController_h__
#define __ViewListController_h__

#include <map>
#include <nanogui/widget.h>
#include <ostream>
#include <string>

class ViewOptions {
  public:
    ViewOptions() : visible(false) {}
    bool visible;
};

class ViewListController {
  public:
    ViewListController() {}
    void set(std::string name, bool vis) { items[name].visible = vis; }
    ViewOptions get(std::string name) {
        // default is for views to be visible except for the patterns window
        auto found = items.find(name);
        if (found == items.end()) {
            set(name, name != "Patterns");
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
