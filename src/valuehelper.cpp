#include "valuehelper.h"
#include <symboltable.h>
#include <unordered_map>

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
    static std::unordered_map<std::string, Value::Kind> property_types;
    if (property_types.empty()) {
    	property_types["alignment"] = Value::t_integer;
		property_types["auto_update"] = Value::t_bool;
    	property_types["behaviour"] = Value::t_integer;
    	property_types["bg_color"] = Value::t_string;
    	property_types["bg_on_color"] = Value::t_string;
    	property_types["border"] = Value::t_integer;
    	property_types["caption"] = Value::t_string;
    	property_types["channel"] = Value::t_string;
    	property_types["command"] = Value::t_string;
    	property_types["connection"] = Value::t_string;
    	property_types["display_grid"] = Value::t_bool;
    	property_types["file_name"] = Value::t_string;
    	property_types["fg_color"] = Value::t_string;
    	property_types["font_size"] = Value::t_integer;
    	property_types["format"] = Value::t_string;
    	property_types["grid_intensity"] = Value::t_float;
    	property_types["height"] = Value::t_integer;
		property_types["items"] = Value::t_string;
		property_types["items_file"] = Value::t_string;
    	property_types["image"] = Value::t_string;
    	property_types["image_alpha"] = Value::t_float;
    	property_types["image_file"] = Value::t_string;
    	property_types["inverted_visibility"] = Value::t_bool;
    	property_types["on_caption"] = Value::t_string;
    	property_types["text_on_colour"] = Value::t_string;
    	property_types["overlay_plots"] = Value::t_bool;
    	property_types["pos_x"] = Value::t_integer;
    	property_types["pos_y"] = Value::t_integer;
    	property_types["remote"] = Value::t_string;
    	property_types["scale"] = Value::t_float;
    	property_types["screen_height"] = Value::t_integer;
    	property_types["screen_id"] = Value::t_string;
    	property_types["screen_width"] = Value::t_integer;
		property_types["selected"] = Value::t_string;
		property_types["selected_index"] = Value::t_integer;
    	property_types["tab_position"] = Value::t_integer;
    	property_types["text"] = Value::t_string;
    	property_types["text_colour"] = Value::t_string;
    	property_types["text_colour"] = Value::t_string;
    	property_types["theme"] = Value::t_string;
    	property_types["valign"] = Value::t_integer;
    	property_types["value_scale"] = Value::t_float;
    	property_types["value_type"] = Value::t_integer;
    	property_types["visibility"] = Value::t_bool;
    	property_types["width"] = Value::t_integer;
		property_types["working_text"] = Value::t_string;
    	property_types["wrap"] = Value::t_bool;
    	property_types["x_offset"] = Value::t_integer;
    	property_types["x_scale"] = Value::t_float;
    }
    auto found = property_types.find(property);
    if (found != property_types.end()) { return found->second; }
	return Value::t_string;
}

Value defaultForProperty(const std::string &property) {
    static std::unordered_map<std::string, Value> property_defaults;
    if (property_defaults.empty()) {
    	property_defaults["alignment"] = 1;
		property_defaults["auto_update"] = false;
    	property_defaults["behaviour"] = 1;
    	property_defaults["bg_color"] = "0.7,0.7,0.7,1.0";
    	property_defaults["bg_on_color"] = "1.0,1.0,1.0,1.0";
    	property_defaults["border"] = 0;
    	property_defaults["border_grad_top"] = "0.8,0.8,0.8,1.0";
    	property_defaults["border_grad_bot"] = "0.2,0.2,0.2,1.0";
    	property_defaults["border_style"] = 1;
    	property_defaults["border_colouring"] = 0;
    	property_defaults["border_grad_dir"] = 0;
    	property_defaults["caption"] = "";
    	property_defaults["channel"] = Value{};
    	property_defaults["command"] = Value{};
    	property_defaults["connection"] = Value{};
    	property_defaults["display_grid"] = false;
    	property_defaults["fg_color"] = "0.3,0.3,0.7,1.0";
    	property_defaults["file_name"] = Value{};
    	property_defaults["font_size"] = 24;
    	property_defaults["format"] = Value{};
    	property_defaults["grid_intensity"] = 0.05;
    	property_defaults["height"] = 48;
    	property_defaults["image"] = Value{};
    	property_defaults["image_alpha"] = 1.0;
    	property_defaults["image_file"] = Value{};
    	property_defaults["inverted_visibility"] = false;
		property_defaults["items"] = "";
		property_defaults["items_file"] = "";
    	property_defaults["on_caption"] = "";
    	property_defaults["overlay_plots"] = true;
    	property_defaults["pos_x"] = 0;
    	property_defaults["pos_y"] = 0;
    	property_defaults["remote"] = Value{};
    	property_defaults["scale"] = 1.0;
    	property_defaults["screen_height"] = 600;
    	property_defaults["screen_id"] = 0;
    	property_defaults["screen_width"] = 800;
		property_defaults["selected"] = "";
		property_defaults["selected_index"] = -1;
    	property_defaults["tab_position"] = 0;
    	property_defaults["text"] = "";
    	property_defaults["text_colour"] = "0.2,0.2,0.2,1.0";
    	property_defaults["text_colour"] = "0.2,0.2,0.2,1.0";
    	property_defaults["text_on_colour"] = "0.2,0.2,0.2,1.0";
    	property_defaults["theme"] = "";
    	property_defaults["valign"] = 1;
    	property_defaults["value_scale"] = 1.0f;
    	property_defaults["value_type"] = 0;
    	property_defaults["visibility"] = true;
    	property_defaults["width"] = 72;
    	property_defaults["wrap"] = false;
    	property_defaults["x_offset"] = 0;
    	property_defaults["x_scale"] = 1.0;
    }
    auto found = property_defaults.find(property);
    if (found != property_defaults.end()) { return found->second; }
    return defaultForType(typeForProperty(property));
}

std::string format_caption(const std::string &caption, const std::string format_string, int value_type, float value_scale) {
    std::string valStr(caption);
    float scale = value_scale;
    if (scale == 0.0f) scale = 1.0f;
    if (format_string.length()) {
      if (value_type == Value::t_integer) {// integer
        char buf[20];
        long val = std::atol(valStr.c_str());
        snprintf(buf, 20, format_string.c_str(), (long)(val / scale));
        valStr = buf;
      }
      else if (value_type == Value::t_float) {
        char buf[20];
        float val = std::atof(valStr.c_str());
        snprintf(buf, 20, format_string.c_str(), val / scale);
        valStr = buf;
      }
    }
    else if (value_type == Value::t_float) {
        char buf[20];
        float val = std::atof(valStr.c_str());
        snprintf(buf, 20, "%5.3f", val / scale);
        valStr = buf;
    }
    else if (value_type == Value::t_integer) {
        char buf[20];
        long val = std::atol(valStr.c_str());
        snprintf(buf, 20, "%ld", (long)(val / scale));
        valStr = buf;
    }
	return valStr;
}
