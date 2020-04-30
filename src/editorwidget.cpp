//
//  EditorWidget.cpp
//  Project: humid
//
//  All rights reserved. Use of this source code is governed by the
//  3-clause BSD License in LICENSE.txt.

#include <iostream>
#include <string>
#include <assert.h>
#include <nanogui/common.h>
#include <nanogui/widget.h>

#include "editorwidget.h"
#include "selectable.h"
#include "connectable.h"
#include "linkableobject.h"
#include "palette.h"
#include "editor.h"
#include "editorgui.h"
#include "userwindow.h"
#include "structure.h"
#include "draghandle.h"
#include "propertywindow.h"
#include "themewindow.h"
#include "helper.h"
#include "editorlabel.h"

extern Handle::Mode all_handles[];

EditorWidget::EditorWidget(NamedObject *owner, const std::string structure_name, nanogui::Widget *w, LinkableProperty *lp)
  : Selectable(0), EditorObject(owner), Connectable(lp), base(structure_name), dh(0), handles(9), handle_coordinates(9,2),
    definition(0), value_scale(1.0f), tab_position(0), visibility(0), inverted_visibility(0), border(1), value_type(0) {
    assert(w != 0);
    Palette *p = dynamic_cast<Palette*>(w);
    if (!p) {
      p = EDITOR->gui()->getUserWindow();
    }
    palette = p;
}

EditorWidget::EditorWidget(NamedObject *owner, const std::string structure_name, const std::string &nam,
      nanogui::Widget *w, LinkableProperty *lp)
  : Selectable(0), EditorObject(owner, nam), Connectable(lp), base(structure_name), dh(0), handles(9), handle_coordinates(9,2),
  definition(0), value_scale(1.0f), tab_position(0), visibility(0), inverted_visibility(0), border(1), value_type(0) {
    assert(w != 0);
    Palette *p = dynamic_cast<Palette*>(w);
    if (!p) {
      p = EDITOR->gui()->getUserWindow();
    }
    palette = p;
}

EditorWidget::~EditorWidget() { }

const std::string &EditorWidget::getValueFormat() {
  return format_string;
}

void EditorWidget::setValueFormat(const std::string fmt) {
    format_string = fmt;
}

int EditorWidget::getValueType() {
  return value_type;
}

void EditorWidget::setValueType(int fmt) {
    value_type = fmt;
}

void EditorWidget::setVisibilityLink(LinkableProperty *lp) {
  if(visibility) visibility->unlink(this);
  visibility = lp;
  if (visibility) visibility->link(new LinkableVisibility(this));
}

EditorWidget *EditorWidget::create(const std::string kind) {
    return nullptr; // TBD
}

void EditorWidget::addLink(Link *new_link) { links.push_back(*new_link); }

void EditorWidget::addLink(const Link &new_link) {
  links.push_back(new_link);
}

void EditorWidget::removeLink(Anchor *src, Anchor *dest) {
  std::list<Link>::iterator iter = links.begin();
  while (iter != links.end()) {
    const Link &l = *iter;
    if (l.source == src && l.dest == dest)
      iter = links.erase(iter);
    else
      ++iter;
  }
}
void EditorWidget::updateLinks() {
  std::list<Link>::iterator iter = links.begin();
  while (iter != links.end()) {
    const Link &l = *iter++;
    l.update();
  }
}


bool EditorWidget::editorMouseButtonEvent(nanogui::Widget *widget, const nanogui::Vector2i &p, int button, bool down, int modifiers) {

    using namespace nanogui;

    if (EDITOR->isEditMode()) {
        if (down) {
            if (!mSelected && !(modifiers & GLFW_MOD_SHIFT) ) palette->clearSelections();
            if (mSelected) {
              if (modifiers & GLFW_MOD_SHIFT) {
                deselect();
              }
            }
            else
                select();
        }
        return false;
    }
    else {
        if (EDITOR->getDragHandle()) EDITOR->getDragHandle()->setVisible(false);
    }
    return true; // caller should continue to call the default handler for the object
}

bool EditorWidget::editorMouseMotionEvent(nanogui::Widget *widget, const nanogui::Vector2i &p,
        const nanogui::Vector2i &rel, int button, int modifiers) {
    if ( !EDITOR->isEditMode() ) {
        EDITOR->getDragHandle()->setVisible(false);
        return true; // caller should continue to call the default handler for the object
    }

    Eigen::Vector2d pt(p.x(), p.y());

    Eigen::VectorXd distances = (handle_coordinates.rowwise() - pt.transpose()).rowwise().squaredNorm();

    double min = distances.row(0).x();

    int idx = 0;

    for (int i=1; i<9; ++i) {
        if (distances.row(i).x() < min) {
            min = distances.row(i).x();
            idx = i;
        }
    }

    nanogui::DragHandle *drag_handle = EDITOR->getDragHandle();
    if (handles[idx].mode() != Handle::NONE) {
        drag_handle->setTarget(widget);

        drag_handle->setPosition(
                Vector2i(handles[idx].position().x() - drag_handle->size().x()/2,
                            handles[idx].position().y() - drag_handle->size().y()/2) ) ;

        if (drag_handle->propertyMonitor())
            drag_handle->propertyMonitor()->setMode( handles[idx].mode() );

        updateHandles(widget);
        drag_handle->setVisible(true);
    }
    else {
        drag_handle->setPosition(
                Vector2i(widget->position().x() + widget->width() - drag_handle->size().x()/2,
                            widget->position().y() + widget->height() - drag_handle->size().y()/2) ) ;

        updateHandles(widget);
        drag_handle->setVisible(true);
    }

    return false;
}

bool EditorWidget::editorMouseEnterEvent(nanogui::Widget *widget, const Vector2i &p, bool enter) {
    if (enter) updateHandles(widget);
    else {
        EDITOR->getDragHandle()->setVisible(false);
        updateHandles(widget);
    }

    return true;
}

void EditorWidget::updateHandles(nanogui::Widget *w) {
    for (int i=0; i<9; ++i) {
        Handle h = Handle::create(all_handles[i], w->position(), w->size());
        handle_coordinates(i,0) = h.position().x();
        handle_coordinates(i,1) = h.position().y();
        handles[i] = h;
    }
}

void EditorWidget::drawSelectionBorder(NVGcontext *ctx, nanogui::Vector2i pos, nanogui::Vector2i size) {
    if (mSelected) {
        nvgStrokeWidth(ctx, 4.0f);
        nvgBeginPath(ctx);
        nvgRect(ctx, pos.x(), pos.y(), size.x(), size.y());
        nvgStrokeColor(ctx, nvgRGBA(80, 220, 0, 255));
        nvgStroke(ctx);
    }
}

void EditorWidget::drawElementBorder(NVGcontext *ctx, nanogui::Vector2i pos, nanogui::Vector2i size) {
  if (EDITOR->isEditMode()) {
    nvgStrokeWidth(ctx, 4.0f);
    nvgBeginPath(ctx);
    nvgRect(ctx, pos.x(), pos.y(), size.x(), size.y());
    nvgStrokeColor(ctx, nvgRGBA(192, 192, 255, 255));
    nvgStroke(ctx);
  }
}

std::string EditorWidget::baseName() const {
    return base;
}

const std::string &EditorWidget::getName() const { return name; }

void EditorWidget::setPatterns(const std::string patterns) { pattern_list = patterns; }
const std::string &EditorWidget::patterns() const { return pattern_list; }

void EditorWidget::setDefinition(Structure *defn) { definition = defn; }
Structure *EditorWidget::getDefinition() { return definition; }

float EditorWidget::valueScale() { return value_scale; }
void EditorWidget::setValueScale(float s) { value_scale = s;}

int EditorWidget::tabPosition() { return tab_position; }
void EditorWidget::setTabPosition(int p) { tab_position = p; }


void EditorWidget::justSelected() {
  EDITOR->gui()->getUserWindow()->select(this);
  PropertyWindow *prop = EDITOR->gui()->getPropertyWindow();
  if (prop) {
    //prop->show(*getWidget());
    prop->update();
  }
  ThemeWindow *tw = EDITOR->gui()->getThemeWindow();
  nanogui::Widget *w = dynamic_cast<nanogui::Widget*>(this);
  if (w && tw && tw->getWindow()->visible() ) {
    tw->loadTheme(w->theme());
  }
}

void EditorWidget::justDeselected() {
  EDITOR->gui()->getUserWindow()->deselect(this);
  if (dynamic_cast<nanogui::Widget*>(this)) {
    updateStructure(); // save any changes to its structure
  }
  ThemeWindow *tw = EDITOR->gui()->getThemeWindow();
  nanogui::Widget *w = dynamic_cast<nanogui::Widget*>(EDITOR->gui()->getUserWindow()->getWindow());
  if (w && tw && tw->getWindow()->visible() ) {
    tw->loadTheme(w->theme());
  }
  PropertyWindow *prop = EDITOR->gui()->getPropertyWindow();
  if (prop) {
    prop->update();
    EDITOR->gui()->needsUpdate();
  }
}
void EditorWidget::getPropertyNames(std::list<std::string> &names) {
  names.push_back("Border");
  names.push_back("Connection");
  names.push_back("Font Size");
  names.push_back("Format");
  names.push_back("Height");
  names.push_back("Horizontal Pos");
  names.push_back("Inverted Visibility");
  names.push_back("Name"); // not common
  names.push_back("Remote");
  names.push_back("Structure");
  names.push_back("Tab Position");
  names.push_back("Value Scale");
  names.push_back("Value Type");
  names.push_back("Vertical Pos");
  names.push_back("Visibility");
  names.push_back("Width");
}

void EditorWidget::setPropertyValue(const std::string &prop, const Value &v) {
  setProperty(prop, v.asString());
}

void EditorWidget::setProperty(const std::string &prop, const std::string value) {
  std::string::size_type sz;
  if (prop == "Horizontal Pos") {
    nanogui::Widget *w = dynamic_cast<nanogui::Widget*>(this);
    if (w) w->setPosition( nanogui::Vector2i(std::stoi(value,&sz),w->position().y()) );
    return;
  }
  if (prop == "Vertical Pos") {
    nanogui::Widget *w = dynamic_cast<nanogui::Widget*>(this);
    if (w) w->setPosition( nanogui::Vector2i(w->position().x(), std::stoi(value,&sz)) );
    return;
  }
  if (prop == "Width") {
    nanogui::Widget *w = dynamic_cast<nanogui::Widget*>(this);
    if (w) w->setWidth(std::stoi(value,&sz));
    return;
  }
  if (prop == "Height") {
    nanogui::Widget *w = dynamic_cast<nanogui::Widget*>(this);
    if (w) w->setHeight(std::stoi(value,&sz));
    return;
  }
  if (prop == "Font Size") {
    nanogui::Widget *w = dynamic_cast<nanogui::Widget*>(this);
    if (w) w->setFontSize(std::stoi(value,&sz));
    return;
  }
  if (prop == "Format") {
    format_string = value;
    return;
  }
  if (prop == "Value Type") {
    value_type = std::atoi(value.c_str());
    return;
  }
  if (prop == "Value Scale") {
    float scale = std::atof(value.c_str());
    if (scale != 0.0) setValueScale(scale);
    return;
  }
  if (prop == "Tab Position") {
    int pos = std::atoi(value.c_str());
    if (pos != 0) setTabPosition(pos);
    return;
  }
  if (prop == "Border") {
    border = std::atoi(value.c_str());
    return;
  }
  if (prop == "Remote") {
    remote_name = value;
    LinkableProperty *remote_prop = EDITOR->gui()->findLinkableProperty(value);
    if (remote)
      remote->unlink(this);
    remote = remote_prop;
    // note: remote->link() not yet called. see subclass method.
  }
  if (prop == "Connection") {
    connection_name = value;
  }
  if (prop == "Visibility" && !value.empty()) {
    if (visibility) visibility->unlink(this);
    visibility = EDITOR->gui()->findLinkableProperty(value);
    if (visibility) visibility->link(new LinkableVisibility(this));
  }
  if (prop == "Inverted Visibility" && !value.empty()) {
    inverted_visibility = (value == "1" || value == "true" || value == "TRUE");    
  }

}

Value EditorWidget::getPropertyValue(const std::string &prop) {
  nanogui::Widget *w = dynamic_cast<nanogui::Widget*>(this);
  if (!w) {
    std::cout << "Error: " << name << " does not seem to be a widget\n";
    return SymbolTable::Null;
  }
  if (prop == "Structure") return base;
  if (prop == "Horizontal Pos") {
    nanogui::Widget *w = dynamic_cast<nanogui::Widget*>(this);
    return (w) ? w->position().x() : 0;
  }
  if (prop == "Vertical Pos") {
    nanogui::Widget *w = dynamic_cast<nanogui::Widget*>(this);
    return (w) ? w->position().y() : 0;
  }
  if (prop == "Width") {
    nanogui::Widget *w = dynamic_cast<nanogui::Widget*>(this);
    return (w) ? w->width() : 0;
  }
  if (prop == "Height") {
    nanogui::Widget *w = dynamic_cast<nanogui::Widget*>(this);
    return (w) ? w->height() : 0;
  }
  if (prop == "Font Size") {
    nanogui::Widget *w = dynamic_cast<nanogui::Widget*>(this);
    return (w) ? w->fontSize() : 0;
  }
  if (prop == "Format") {
    return Value(getValueFormat(), Value::t_string);
  }
  if (prop == "Value Type") {
    return getValueType();
  }
  if (prop == "Value Scale") {
    return valueScale();
  }
  if (prop == "Border") {
    return border;
  }
  if (prop == "Remote") {
    return Value(remote ? remote->tagName() : remote_name, Value::t_string);
  }
  if (prop == "Connection") {
    return Value(remote ? remote->group() : connection_name, Value::t_string);
  }
  if (prop == "Visibility") {
    return Value(visibility ? visibility->tagName() : "", Value::t_string);
  }
  if (prop == "Inverted Visibility") { 
    return inverted_visibility;
  }
  return SymbolTable::Null;
}

std::string EditorWidget::getProperty(const std::string &prop) {
  Value res = getPropertyValue(prop);
  if (res == SymbolTable::Null) return "";
  return res.asString();
}

void EditorWidget::loadPropertyToStructureMap(std::map<std::string, std::string> &property_map) {
  property_map["Border"] = "border";
  property_map["Connection"] = "connection";
  property_map["Font Size"] = "font_size";
  property_map["Format"] = "format";
  property_map["Height"] = "height";
  property_map["Horizontal Pos"] = "pos_x";
  property_map["Inverted Visibility"] = "inverted_visibility";
  property_map["Remote"] = "remote";
  property_map["Structure"] = ""; // not to be copied
  property_map["Tab Position"] = "tab_position";
  property_map["Value Scale"] = "value_scale";
  property_map["Value Type"] = "value_type";
  property_map["Vertical Pos"] = "pos_y";
  property_map["Visibility"] = "visibility";
  property_map["Width"] = "width";
}

// generate or update structure properties from the widget
void EditorWidget::updateStructure() {
  assert(dynamic_cast<nanogui::Widget*>(this));

  StructureClass *sc = findClass(base);
  Structure *s = definition;
  if (!sc)
    sc = createStructureClass(base);
  if (!s) {
    if (base == "SCREEN")
      s = createScreenStructure();
    else
      s = sc->instantiate(nullptr);
    if (s) definition = s;
  }
  if (!s) return;
  s->setStructureDefinition(sc);
  std::list<std::string> property_names;
  std::map<std::string, std::string> property_map;
  loadPropertyToStructureMap(property_map);
  getPropertyNames(property_names);
  for (auto item : property_names) {
    Value v = getPropertyValue(item);
    auto found = property_map.find(item);
    if (v != SymbolTable::Null) {
      if (found != property_map.end()) {
        if ( (*found).second.empty()) {
          std::cout << s->getName() << " skipping update of widget property " << item << "\n";
          continue;
        }
        std::string mapped((*found).second.c_str());
        if (mapped == "width" && v.kind == Value::t_integer && v.iValue < 40)
          v = 40;
        if (mapped == "height" && v.kind == Value::t_integer && v.iValue < 20)
          v = 20;
        s->getProperties().add(mapped, v);
      }
      else {
        std::cout << s->getName() << " setting unmapped property " << item << " to " << v << "\n";
        s->getProperties().add(item.c_str(), v);
      }
    }
  }
}
