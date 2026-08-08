/*
	All rights reserved. Use of this source code is governed by the
	3-clause BSD License in LICENSE.txt.
*/

#ifndef __clockwork__MessageEncoding__
#define __clockwork__MessageEncoding__

#include "cJSON.h"
#include "symboltable.h"
#include "value.h"
#include <boost/none.hpp>
#include <string>
#include <vector>
#include <boost/optional.hpp>

namespace MessageEncoding {
    std::string encodeCommand(std::string cmd, const std::list<Value> &params);
    std::string encodeCommand(std::string cmd, Value p1 = SymbolTable::Null,
                               Value p2 = SymbolTable::Null, Value p3 = SymbolTable::Null,
                               Value p4 = SymbolTable::Null);
    std::string encodeCommand(std::string cmd,
                    boost::optional<std::string> p1,
                    boost::optional<std::string> p2 = boost::none,
                    boost::optional<std::string> p3 = boost::none,
                    boost::optional<std::string> p4 = boost::none);
    std::string encodeState(const std::string &machine, const std::string &new_state,
                             uint64_t authority);
    std::string encodeState(const std::string &machine, const std::string &new_state);
    std::string encodeError(const char *error);
    bool getCommand(const char *msg, std::string &cmd, std::list<Value> **params);
    bool getCommand(const char *msg, std::string &cmd, std::vector<Value> **params);
    bool getState(const char *msg, std::string &cmd, std::list<Value> **params);

    std::string valueType(const Value &v);
    void addValueToJSONObject(cJSON *obj, const char *name, const Value &val);
    void addValueToJSONArray(cJSON *arr, const Value &val);
    Value valueFromJSONObject(cJSON *obj, cJSON *cjType);
}

#endif /* defined(__clockwork__MessageEncoding__) */
