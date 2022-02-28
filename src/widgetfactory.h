#pragma once

#include "editorgui.h"
#include <nanogui/theme.h>
#include <nanogui/widget.h>

#include "linkableproperty.h"
#include "structure.h"
#include <string>
#include <value.h>

struct WidgetParams {
    Structure *s;
    nanogui::Widget *window;
    Structure *element;
    LinkableProperty *lp;
    EditorGUI *gui;
    long font_size;
    const Value &format_val;
    const Value &connection;
    const Value &vis;
    const Value &scale_val;
    const Value &border;
    const Value &auto_update;
    const Value &working_text;
    const Value &font_size_val;
    const Value &remote_name;
    const Value &wrap_v;
    const Value &ivis_v;
    const Value &value_type_val;
    const Value &tab_pos_val;
    const Value &x_scale_val;
    const Value &theme_name;
    Value remote;
    long value_type;
    //const Value &scale_val;
    long tab_pos;
    //const Value &border;
    LinkableProperty *visibility;
    const std::string &kind;
    bool wrap;
    bool ivis;
    double x_scale;
    double value_scale;
    nanogui::Vector2i offset = nanogui::Vector2i(0, 0);
    nanogui::ref<nanogui::Theme> theme;
    WidgetParams(Structure *structure, nanogui::Widget *w, Structure *elem, EditorGUI *editor_gui,
                 const nanogui::Vector2i &offset);
    void update();
};

void createLabel(WidgetParams &params);
void createList(WidgetParams &params);
void createImage(WidgetParams &params);
void createProgress(WidgetParams &params);
void createText(WidgetParams &params);
void createPlot(WidgetParams &params);
void createButton(WidgetParams &params);
void createFrame(WidgetParams &params);
void createComboBox(WidgetParams &params);
