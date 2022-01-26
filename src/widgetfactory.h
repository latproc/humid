#pragma once

#include <nanogui/widget.h>
#include <nanogui/theme.h>
#include "editorgui.h"

#include "structure.h"
#include "linkableproperty.h"
#include <value.h>
#include <string>


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
	const Value & auto_update;
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
	nanogui::Vector2i offset = nanogui::Vector2i(0,0);
	nanogui::ref<nanogui::Theme> theme;
	WidgetParams(Structure *structure, nanogui::Widget *w, Structure *elem, EditorGUI *editor_gui, const nanogui::Vector2i &offset);
};

void createLabel(WidgetParams &params);
void createImage(WidgetParams &params);
void createProgress(WidgetParams &params);
void createText(WidgetParams &params);
void createPlot(WidgetParams &params);
void createButton(WidgetParams &params);
void createFrame(WidgetParams &params);
void createComboBox(WidgetParams &params);

