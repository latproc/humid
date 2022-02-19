//
//  PanelScreen.h
//  Project: Humid
//
//	All rights reserved. Use of this source code is governed by the
//	3-clause BSD License in LICENSE.txt.

#ifndef __PanelScreen_h__
#define __PanelScreen_h__

#include <list>
#include <nanogui/widget.h>
#include <ostream>
#include <string>

class PanelScreen {
  public:
    PanelScreen(const std::string sname);
    PanelScreen(const PanelScreen &orig);
    virtual ~PanelScreen() {}
    PanelScreen &operator=(const PanelScreen &other);
    std::ostream &operator<<(std::ostream &out) const;
    bool operator==(const PanelScreen &other);

    void setName(const std::string &n) { name = n; }
    std::string getName() { return name; }
    void add(nanogui::Widget *w) {
        w->incRef();
        items.push_back(w);
    }
    void remove(nanogui::Widget *w) {
        items.remove(w);
        w->decRef();
    }

  private:
    std::list<nanogui::Widget *> items;
    std::string name;
};

std::ostream &operator<<(std::ostream &out, const PanelScreen &m);

//
#endif
