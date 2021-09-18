//
//  EditorLinePlot.cpp
//  Project: humid
//
//	All rights reserved. Use of this source code is governed by the
//	3-clause BSD License in LICENSE.txt.

#include <iostream>

#define NANOVG_GL3_IMPLEMENTATION
#include <nanogui/common.h>
#include <nanogui/opengl.h>
#include <nanogui/glutil.h>
#include <nanovg_gl.h>

#include "userwindow.h"
#include "editorlineplot.h"
#include "editor.h"
#include "propertyformhelper.h"
#include "objectwindow.h"
#include "factorybuttons.h"

EditorLinePlot::EditorLinePlot(NamedObject *owner, Widget *parent, std::string nam, LinkableProperty *lp, int icon)
: LinePlot(parent, "Test"), EditorWidget(owner, "PLOT", nam, this, lp), handle_coordinates(9,2),
    start_trigger_value(0), stop_trigger_value(0)
{
}

bool EditorLinePlot::mouseButtonEvent(const nanogui::Vector2i &p, int button, bool down, int modifiers) {

    using namespace nanogui;

    if (editorMouseButtonEvent(this, p, button, down, modifiers))
        return nanogui::LinePlot::mouseButtonEvent(p, button, down, modifiers);

    return true;
}

bool EditorLinePlot::mouseMotionEvent(const nanogui::Vector2i &p, const nanogui::Vector2i &rel, int button, int modifiers) {

    if (editorMouseMotionEvent(this, p, rel, button, modifiers))
        return nanogui::LinePlot::mouseMotionEvent(p, rel, button, modifiers);

    return true;
}

bool EditorLinePlot::mouseEnterEvent(const Vector2i &p, bool enter) {

    if (editorMouseEnterEvent(this, p, enter))
        return nanogui::LinePlot::mouseEnterEvent(p, enter);

    return true;
}

void EditorLinePlot::draw(NVGcontext *ctx) {
    nanogui::LinePlot::draw(ctx);
    if (mSelected) {
        nvgStrokeWidth(ctx, 4.0f);
        nvgBeginPath(ctx);
        nvgRect(ctx, mPos.x(), mPos.y(), mSize.x(), mSize.y());
        nvgStrokeColor(ctx, nvgRGBA(80, 220, 0, 255));
        nvgStroke(ctx);
    }
}

const std::map<std::string, std::string> & EditorLinePlot::property_map() const {
  auto structure_class = findClass("PLOT");
  assert(structure_class);
  return structure_class->property_map();
}

const std::map<std::string, std::string> & EditorLinePlot::reverse_property_map() const {
  auto structure_class = findClass("PLOT");
  assert(structure_class);
  return structure_class->reverse_property_map();
}

void EditorLinePlot::loadPropertyToStructureMap(std::map<std::string, std::string> &properties) {
  properties = property_map();
}

void EditorLinePlot::getPropertyNames(std::list<std::string> &names) {
    EditorWidget::getPropertyNames(names);
    names.push_back("X scale");
    names.push_back("X offset");
    names.push_back("Grid Intensity");
    names.push_back("Display Grid");
    names.push_back("Overlay plots");
    //names.push_back("Line style");
    //names.push_back("Line thickness");
}

Value EditorLinePlot::getPropertyValue(const std::string &prop) {
  Value res = EditorWidget::getPropertyValue(prop);
  if (res != SymbolTable::Null)
    return res;
  if (prop == "X scale") {
    return x_scale;
  }
  if (prop == "X offset") {
    return x_scroll;
  }
  if (prop == "Grid Intensity") {
    return grid_intensity;
  }
  if (prop == "Display Grid") {
    return display_grid;
  }
  if (prop == "Overlay plots") {
    return overlay_plots;
  }
  if (prop == "Line style") {
    //return (int)series->getLineStyle();
  }
  if (prop == "Line thickness") {
    //series->getLineWidth()
  }
  return SymbolTable::Null;
}

void EditorLinePlot::setTriggerValue(UserWindow *user_window, SampleTrigger::Event evt, int val) {
  if (evt == SampleTrigger::START) start_trigger_value = val;
  else if (evt == SampleTrigger::STOP) stop_trigger_value = val;
}

void EditorLinePlot::setTriggerName(UserWindow *user_window, SampleTrigger::Event evt, const std::string name) {

  if (!user_window) return;

  CircularBuffer *buf = user_window->getDataBuffer(name);
  if (!buf) return;
  SampleTrigger *t = buf->getTrigger(evt);
  if (!t) {
    t = new SampleTrigger(name, 0);
    if (evt == SampleTrigger::START) t->setTriggerValue(start_trigger_value);
    else if (evt == SampleTrigger::STOP) t->setTriggerValue(stop_trigger_value);
    buf->setTrigger(t, evt);
  }
  else
    t->setPropertyName(name);
}

void EditorLinePlot::loadProperties(PropertyFormHelper* properties) {
  EditorWidget::loadProperties(properties);
  EditorGUI *gui = EDITOR->gui();

  properties->addVariable<float> ("X scale",
    [&](float value) mutable{ x_scale = value; },
    [&]()->float { return x_scale; });
  properties->addVariable<float> ("X offset",
    [&](float value) mutable{ x_scroll = value; },
    [&]()->float { return x_scroll; });
  properties->addVariable<float> ("Grid Intensity",
    [&](float value) mutable{ grid_intensity = value; },
    [&]()->float { return grid_intensity; });
  properties->addVariable<bool> ("Display Grid",
    [&](bool value) mutable{ display_grid = value; },
    [&]()->bool { return display_grid; });
  properties->addVariable<bool> ("Overlay plots",
    [&](bool value) mutable{ overlay_plots = value; },
    [&]()->bool { return overlay_plots; });
  properties->addVariable<std::string> ("Monitors",
    [&, gui](std::string value) mutable{
      setMonitors( gui->getUserWindow(), value); },
    [&, gui]()->std::string{ return monitors(); });
  properties->addButton("Monitor Selected", [&,gui]() mutable{
    if (gui->getObjectWindow()->hasSelections()) {
      requestFocus();
      std::string items;
      for (auto sel : gui->getObjectWindow()->getSelected()) {
        if (items.length()) items += ",";
        ObjectFactoryButton *btn = dynamic_cast<ObjectFactoryButton*>(sel);
        if (btn) {
          items += btn->tagName();
        }
      }
      if (items.length()) {
        setMonitors( gui->getUserWindow(), items );
        gui->getObjectWindow()->clearSelections();
      }

      gui->getPropertyWindow()->update();
    }
  });
  properties->addButton("Save Data", [&,gui,this]() {
    std::string file_path( nanogui::file_dialog(
        { {"csv", "Comma separated values"},
          {"txt", "Text file"} }, true));
    if (file_path.length())
      this->saveData(file_path);
  });
  properties->addButton("Clear Data", [&]() {
    for (auto *ts : data) {
      ts->getData()->clear();
    }
  });
  for (auto series : data) {
    properties->addGroup(series->getName());
    properties->addVariable<int> (
      "Line style",
      [&,series](int value) mutable{
        series->setLineStyle(static_cast<nanogui::TimeSeries::LineStyle>(value));
      },
      [&,series]()->int{ return (int)series->getLineStyle(); });
    properties->addVariable<float> (
      "Line thickness",
      [&,series](float value) mutable{
        series->setLineWidth(value);
      },
      [&,series]()->float{ return series->getLineWidth(); });
    properties->addGroup("Remote");
    properties->addVariable<std::string> (
      "Remote object",
      [&](std::string value) { setRemoteName(value); },
      [&]()->std::string{ LinkableProperty *lp = EDITOR->gui()->findLinkableProperty(series->getName()); return lp ? lp->tagName() : ""; });
    properties->addVariable<std::string> (
      "Connection",
      [&,this,properties](std::string value) {
        if (remote) remote->setGroup(value); else setConnection(value);
       },
      [&]()->std::string{ return remote ? remote->group() : getConnection(); });
    properties->addVariable<std::string> (
      "Visibility",
      [&,this,properties](std::string value) {
        LinkableProperty *lp = EDITOR->gui()->findLinkableProperty(value);
        if (visibility) visibility->unlink(this);
        visibility = lp;
        if (lp) { lp->link(new LinkableVisibility(this)); }
       },
      [&]()->std::string{ return visibility ? visibility->tagName() : ""; });
  }
  properties->addGroup("Triggers");
  properties->addVariable<std::string> (
      "Start trigger",
      [&,gui](const std::string value) mutable{
      setTriggerName(gui->getUserWindow(),
      SampleTrigger::START, value);},
      [&]()->std::string { return start_trigger_name; });
  properties->addVariable<int> ("Start Value",
      [&,gui](int value) mutable{
        setTriggerValue(gui->getUserWindow(),
        SampleTrigger::START, value); },
        [&]()->int { return start_trigger_value; });
  properties->addVariable<std::string> ("Stop trigger",
      [&,gui](const std::string value) mutable{
        setTriggerName(gui->getUserWindow(),
        SampleTrigger::STOP, value);},
        [&,gui]()->std::string { return start_trigger_name; });
  properties->addVariable<int> ("Stop Value",
      [&](int value) mutable{
        setTriggerValue(gui->getUserWindow(),
        SampleTrigger::STOP, value); },
        [&]()->int { return stop_trigger_value; });
}

