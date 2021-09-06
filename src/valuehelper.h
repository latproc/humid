#pragma once

#include <value.h>

Value defaultForType(Value::Kind kind);
Value::Kind typeForProperty(const std::string &name);
Value defaultForProperty(const std::string &name);