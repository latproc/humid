/*
	All rights reserved. Use of this source code is governed by the
	3-clause BSD License in LICENSE.txt.
*/

#include <value.h>

#include <nanogui/screen.h>
#include <nanogui/window.h>

#include "structure.h"
#include "helper.h"

std::list<Structure *>hm_structures;
std::list<StructureClass *> hm_classes;

StructureClass::StructureClass(const std::string class_name) 
: NamedObject(nullptr, class_name), builtin(false) {}
StructureClass::StructureClass(const std::string class_name, const std::string base_class)
: NamedObject(nullptr, class_name), base(base_class), builtin(false) {};

std::ostream &Structure::operator<<(std::ostream &out) const {
	return out << name << " " << kind;
}

std::ostream &operator<<(std::ostream &out, const Structure &s) {
	return s.operator<<(out);
}

void StructureClass::addProperty(const char *p) {
	property_names.insert(p);
}

void StructureClass::addProperty(const std::string &p) {
	property_names.insert(p.c_str());
}

void StructureClass::addPrivateProperty(const char *p) {
	property_names.insert(p);
	local_properties.insert(p); //
}

void StructureClass::addPrivateProperty(const std::string &p) {
	property_names.insert(p.c_str());
	local_properties.insert(p); //
}

Structure *StructureClass::instantiate(Structure *parent) {
	std::string s_name(NamedObject::nextName(parent));
	Structure *s = new Structure(parent, s_name, name);
	s->getProperties().add(getProperties()); // copy default properties to the new structure
	hm_structures.push_back(s);
	return s;
}

Structure *StructureClass::instantiate(Structure *parent, const std::string s_name) {
	Structure *s = new Structure(parent, s_name, name);
	s->getProperties().add(getProperties()); // copy default properties to the new structure
	hm_structures.push_back(s);
	return s;
}

Structure::Structure(Structure *parent, const std::string sname, const std::string skind)
	: NamedObject(parent, sname), kind(skind), changed_(false), class_definition(0), owner(parent) {
}

/*
Structure *Structure::clone(std::string new_name) {
	Structure *s = new Structure(parent, *this);
	s->name = new_name;
	hm_structures.push_back(s);
	return s;
}
Structure::Structure(Structure *parent, const Structure &other)
	: NamedObject(parent, ""), kind(other.kind), class_definition(other.class_definition), owner(parent) {
	properties.add(other.properties);
	nextName(this);
	hm_structures.push_back(this);
}
*/


bool Structure::isA(const std::string &seek) {
	if (kind == seek) return true;
	return class_definition && (class_definition->getName() == seek || class_definition->getBase() == seek);
}


long Structure::getIntProperty(const std::string name, int default_value) {
	const Value &val = properties.find(name.c_str());
	long res;
	if (val == SymbolTable::Null || !val.asInteger(res))
		return default_value;
	else
		return res;
}

std::string Structure::getStringProperty(const std::string name, const char *default_value) {
	const Value &val = properties.find(name.c_str());
	if (val == SymbolTable::Null)
		return default_value;
	else
		return val.asString();
}

bool Structure::getBoolProperty(const std::string name, bool default_value) {
	const Value &val = properties.find(name.c_str());
	if (val != SymbolTable::Null) {
		switch(val.kind) {
			case Value::t_bool: return val.bValue;
			case Value::t_integer: return val.iValue;
			case Value::t_float: return default_value;
			case Value::t_string:
			case Value::t_symbol: 
				if (val.sValue == "true" || val.sValue == "TRUE") return true;
				else if (val.sValue == "false" || val.sValue == "FALSE") return false;
				else return default_value;
			default:
				return default_value;
		}
	}
	return default_value;
}

bool writePropertyList(std::ostream &out, const SymbolTable &properties) {
	const char *begin_properties = "(\n    ";
	const char *property_delim = ",\n    ";
	const char *delim = begin_properties;
	SymbolTableConstIterator i = properties.begin();
	while (i != properties.end()) {
		auto item = *i++;
		if (item.second != SymbolTable::Null && item.second.asString() != "") {
			if (item.second.kind == Value::t_string)
				out << delim << item.first << ": " << item.second; // quotes are automatically added to string values
			else
				out << delim << item.first << ": " << item.second;
			delim = property_delim;
		}
	}
	if (delim == property_delim) out << ")"; // only output the ')' if there were properties
	return true;
}

bool writeOptions(std::ostream &out,const std::map<std::string, Value> &options) {
	const char *begin_properties = "";
	const char *property_delim = ";\n";
	const char *delim = begin_properties;
	std::map<std::string, Value>::const_iterator i = options.begin();
	while (i != options.end()) {
		auto item = *i++;
		out << delim << "\tOPTION " << item.first << " " << item.second; // quotes are automatically added to string values
		delim = property_delim;
	}
	if (delim == property_delim) out << delim;
	return true;
}

bool Structure::save(std::ostream &out) {
	out << name << " " << kind;
#if 0
	if (class_definition) {
		// only emit properties that differ from the class values
	}
	else
#endif
	bool res = writePropertyList(out, properties);
	out << ";\n";
	return res;
}

bool StructureClass::save(std::ostream &out) {
	using namespace nanogui;
	//if (locals.empty() && properties.begin() == properties.end()) return true;
	out << getName() << " STRUCTURE";
	writePropertyList(out, properties);
	if (!getBase().empty()) out << " EXTENDS " << getBase();
	out << " {\n";
	writeOptions(out, options);
	for (auto local : locals) {
		out << "  ";
		Structure *s = local.machine;
		s->save(out);
	}
	out << "}\n";
	return true;
	// TBD connection groups are not supported yet; these value should be saved to the
	// appropriate connection file, not a screen
#if 0
	{
		const std::map<std::string, LinkableProperty*> &properties(gui->getLinkableProperties());
		std::set<std::string>groups;
		for (auto item : properties) {
			groups.insert(item.second->group());
		}
		for (auto group : groups) {
			out << shortName(group) << " CONNECTION_GROUP (path:\""<< group << "\");\n";
		}
	}
	std::stringstream pending_definitions;
	std::set<LinkableProperty*> used_properties;
	std::string screen_type(getName());
	boost::to_upper(screen_type);
	if (base == "SCREEN")
		out << screen_type << " STRUCTURE EXTENDS SCREEN {\n";
	else
		out << screen_type << " STRUCTURE {\n";
	return false;
#endif
}
