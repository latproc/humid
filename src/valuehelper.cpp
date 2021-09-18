#include "valuehelper.h"
#include <symboltable.h>

Value defaultForType(Value::Kind kind) {
	switch(kind) {
		case Value::t_empty: return SymbolTable::Null;
		case Value::t_integer: return 0;
		case Value::t_string: return "";
		case Value::t_bool: return false;
		case Value::t_symbol: return Value("", kind);
		case Value::t_dynamic: return SymbolTable::Null;
		case Value::t_float: return 0.0;
		default: ;
	}
	return SymbolTable::Null;
}

Value::Kind typeForProperty(const std::string &property) {
	if (property == "alignment") { return Value::t_integer; }
	if (property == "behaviour") { return Value::t_integer; }
	if (property == "bg_color") { return Value::t_string; }
	if (property == "bg_on_color") { return Value::t_string; }
	if (property == "border") { return Value::t_integer; }
	if (property == "caption") { return Value::t_string; }
	if (property == "channel") { return Value::t_string; }
	if (property == "command") { return Value::t_string; }
	if (property == "connection") { return Value::t_string; }
	if (property == "display_grid") { return Value::t_bool; }
	if (property == "file_name") { return Value::t_string; }
	if (property == "font_size") { return Value::t_integer; }
	if (property == "format") { return Value::t_string; }
	if (property == "grid_intensity") { return Value::t_float; }
	if (property == "height") { return Value::t_integer; }
	if (property == "image") { return Value::t_string; }
	if (property == "image_alpha") { return Value::t_float; }
	if (property == "image_file") { return Value::t_string; }
	if (property == "inverted_visibility") { return Value::t_bool; }
	if (property == "on_caption") { return Value::t_string; }
	if (property == "on_text_colour") { return Value::t_string; }
	if (property == "overlay_plots") { return Value::t_bool; }
	if (property == "pos_x") { return Value::t_integer; }
	if (property == "pos_y") { return Value::t_integer; }
	if (property == "remote") { return Value::t_string; }
	if (property == "scale") { return Value::t_float; }
	if (property == "screen_height") { return Value::t_integer; }
	if (property == "screen_id") { return Value::t_string; }
	if (property == "screen_width") { return Value::t_integer; }
	if (property == "tab_position") { return Value::t_integer; }
	if (property == "text") { return Value::t_string; }
	if (property == "text_color") { return Value::t_string; }
	if (property == "text_colour") { return Value::t_string; }
	if (property == "valign") { return Value::t_integer; }
	if (property == "value_scale") { return Value::t_integer; }
	if (property == "value_type") { return Value::t_integer; }
	if (property == "visibility") { return Value::t_bool; }
	if (property == "width") { return Value::t_integer; }
	if (property == "wrap") { return Value::t_bool; }
	if (property == "x_offset") { return Value::t_integer; }
	if (property == "x_scale") { return Value::t_float; }
	return Value::t_string;
}

Value defaultForProperty(const std::string &property) {
	if (property == "alignment") { return 1; }
	if (property == "behaviour") { return 1; }
	if (property == "bg_color") { return "0.7,0.7,0.7,1.0"; }
	if (property == "bg_on_color") { return "1.0,1.0,1.0,1.0"; }
	if (property == "border") { return "0.2,0.2,0.2"; }
	if (property == "caption") { return ""; }
	if (property == "channel") { return {}; }
	if (property == "command") { return {}; }
	if (property == "connection") { return {};}
	if (property == "display_grid") { return false; }
	if (property == "file_name") { return {}; }
	if (property == "font_size") { return 24; }
	if (property == "format") { return {}; }
	if (property == "grid_intensity") { return 0.05; }
	if (property == "height") { return 48; }
	if (property == "image") { return {}; }
	if (property == "image_alpha") { return 1.0; }
	if (property == "image_file") { return {}; }
	if (property == "inverted_visibility") { return false; }
	if (property == "on_caption") { return ""; }
	if (property == "on_text_colour") { return "0.2,0.2,0.2"; }
	if (property == "overlay_plots") { return true; }
	if (property == "pos_x") { return 0; }
	if (property == "pos_y") { return 0; }
	if (property == "remote") { return {}; }
	if (property == "scale") { return 1.0; }
	if (property == "screen_height") { return 600; }
	if (property == "screen_id") { return 0; }
	if (property == "screen_width") { return 800; }
	if (property == "tab_position") { return 0; }
	if (property == "text") { return ""; }
	if (property == "text_color") { return "0.2,0.2,0.2,1.0"; }
	if (property == "text_colour") { return "0.2,0.2,0.2,1.0"; }
	if (property == "valign") { return 1; }
	if (property == "value_scale") { return 1.0; }
	if (property == "value_type") { return -1; }
	if (property == "visibility") { return true; }
	if (property == "width") { return 72; }
	if (property == "wrap") { return false; }
	if (property == "x_offset") { return 0; }
	if (property == "x_scale") { return 1.0; }
    return defaultForType(typeForProperty(property));
}
