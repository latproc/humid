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

#include "editorlineplot.h"

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

void EditorLinePlot::loadPropertyToStructureMap(std::map<std::string, std::string> &property_map) {
  EditorWidget::loadPropertyToStructureMap(property_map);
  property_map["X scale"] = "x_scale";
  property_map["X offset"] = "x_offset";
  property_map["Grid Intensity"] = "grid_intensity";
  property_map["Display Grid"] = "display_grid";
  property_map["Overlay plots"] = "overlay_plots";
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
