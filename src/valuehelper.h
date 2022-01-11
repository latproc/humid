#pragma once

#include <value.h>
#include <string>

Value defaultForType(Value::Kind kind);
Value::Kind typeForProperty(const std::string &name);
Value defaultForProperty(const std::string &name);
std::string format_caption(const std::string &caption, const std::string format_string, int value_type, float value_scale);