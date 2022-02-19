//
//  FactoryButtons.h
//  Project: humid
//
//	All rights reserved. Use of this source code is governed by the
//	3-clause BSD License in LICENSE.txt.

#ifndef __FactoryButtons_h__
#define __FactoryButtons_h__

#include "editorgui.h"
#include "helper.h"
#include "linkableobject.h"
#include "linkableproperty.h"
#include "palette.h"
#include "selectablebutton.h"
#include <nanogui/widget.h>
#include <ostream>
#include <string>

class StructureFactoryButton : public SelectableButton {
  public:
    StructureFactoryButton(EditorGUI *screen, const std::string type_str, Palette *pal,
                           nanogui::Widget *parent, int object_type, const std::string &name,
                           const std::string &addr_str);
    nanogui::Widget *create(nanogui::Widget *container) const override;

  private:
    EditorGUI *gui;
    int kind;
    std::string tag_name;
    std::string address_str;
};

class ObjectFactoryButton : public SelectableButton {
  public:
    ObjectFactoryButton(EditorGUI *screen, const std::string type_str, Palette *pal,
                        nanogui::Widget *parent, LinkableProperty *lp);
    nanogui::Widget *create(nanogui::Widget *container) const override;
    const std::string &tagName() { return properties->tagName(); }
    LinkableProperty *details() { return properties; }

  private:
    EditorGUI *gui;
    LinkableProperty *properties;
};

//
#endif
