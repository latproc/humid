/*
	All rights reserved. Use of this source code is governed by the
	3-clause BSD License in LICENSE.txt.
*/

#include <value.h>

#include <nanogui/screen.h>
#include <nanogui/window.h>
#include <map>
#include <string>

#include "structure.h"
#include "helper.h"
#include "linkmanager.h"
#include "valuehelper.h"
#include "colourhelper.h"

nanogui::Color colourFromProperty(Structure *element, const std::string &prop) {
	return colourFromProperty(element, prop.c_str());
}

nanogui::Color colourFromProperty(Structure *element, const char *prop) {
	Value colour(element->getValue(prop));
	if (colour == SymbolTable::Null) {
		colour = defaultForProperty(prop);
	}
    return colourFromValue(colour);
}

std::list<Structure *>hm_structures;
std::list<Structure *>builtin_structures;
std::list<StructureClass *> hm_classes;

std::list<Structure *> Structure::findStructureClasses(const std::string &class_name) {
	std::list<Structure *> result;
	for (auto structure : hm_structures) {
		if (structure->getKind() == class_name) {
			result.push_back(structure);
		}
	}
	return result;
}

static void invert_property_map(const std::map<std::string, std::string> &normal, std::map<std::string, std::string> & reversed) {
	reversed.clear();
	for (auto & item : normal) {
		reversed.insert({item.second, item.first});
	}
}

static void prepare_class_properties(const std::string & class_name, std::map<std::string, std::string> &properties) {
	properties["Border"] = "border";
	properties["Connection"] = "connection";
	properties["Font Size"] = "font_size";
	properties["Format"] = "format";
	properties["Height"] = "height";
	properties["Horizontal Pos"] = "pos_x";
	properties["Inverted Visibility"] = "inverted_visibility";
	properties["Remote"] = "remote";
	properties["Structure"] = ""; // not to be copied
	properties["Tab Position"] = "tab_position";
	properties["Theme"] = "MainTheme";
	properties["Value Scale"] = "value_scale";
	properties["Value Type"] = "value_type";
	properties["Vertical Pos"] = "pos_y";
	properties["Visibility"] = "visibility";
	properties["Width"] = "width";
	if (class_name == "WIDGET") { return; }
	if (class_name == "BUTTON" || class_name == "INDICATOR") {
		properties["Alignment"] = "alignment";
		properties["Background Colour"] = "bg_color";
		properties["Background on colour"] = "bg_on_color";
		properties["Behaviour"] = "behaviour";
		properties["Border colouring"] = "border_colouring";
		properties["Border gradient dir"] = "border_grad_dir";
		properties["Border grad bot"] = "border_grad_bot";
		properties["Border grad top"] = "border_grad_top";
		properties["Border style"] = "border_style";
		properties["Command"] = "command";
		properties["Enabled"] = "enabled";
		properties["Image opacity"] = "image_alpha";
		properties["Image"] = "image";
		properties["Off text"] = "caption";
		properties["On text"] = "on_caption";
		properties["Text colour"] = "text_colour";
		properties["Text on colour"] = "text_on_colour";
		properties["Vertical Alignment"] = "valign";
		properties["Wrap Text"] = "wrap";
	}
	else if (class_name == "TEXT") {
		properties["Alignment"] = "alignment";
		properties["Auto Update"] = "auto_update";
		properties["Font Size"] = "font_size";
		properties["Text"] = "text";
		properties["Vertical Alignment"] = "valign";
		properties["Wrap Text"] = "wrap";
		properties["Working Text"] = "working_text";
	}
	else if (class_name == "LABEL") {
		properties["Alignment"] = "alignment";
		properties["Background Colour"] = "bg_color";
		properties["Caption"] = "caption";
		properties["Font Size"] = "font_size";
		properties["Text Colour"] = "text_colour";
		properties["Vertical Alignment"] = "valign";
		properties["Wrap Text"] = "wrap";
	}
	else if (class_name == "PLOT") {
		properties["Display Grid"] = "display_grid";
		properties["Grid Intensity"] = "grid_intensity";
		properties["Overlay plots"] = "overlay_plots";
		properties["X offset"] = "x_offset";
		properties["X scale"] = "x_scale";
	}
	else if (class_name == "IMAGE") {
		properties["Image File"] = "image_file";
		properties["Scale"] = "scale";
	}
	else if (class_name == "PROGRESS") {
		properties["Background Colour"] = "bg_color";
		properties["Foreground Colour"] = "fg_color";
		properties["Value"] = "value";
	}
	else if (class_name == "COMBOBOX") {
		properties["Background Colour"] = "bg_color";
		properties["Text Colour"] = "text_colour";
	}
}

StructureClass::StructureClass(const std::string class_name)
: NamedObject(nullptr, class_name), builtin(false) {
	prepare_class_properties(class_name, m_property_map);
	invert_property_map(m_property_map, m_reverse_map);
}
StructureClass::StructureClass(const std::string class_name, const std::string base_class)
: NamedObject(nullptr, class_name), base(base_class), builtin(false) {
	prepare_class_properties(class_name, m_property_map);
	invert_property_map(m_property_map, m_reverse_map);
};

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
	return class_definition && (class_definition->isExtension(seek));
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

const Value &Structure::getDefault(const char *name) {
	const auto & result =  class_definition ? class_definition->getDefaults().find(name) : SymbolTable::Null;
	return result;;
}

const Value &Structure::getValue(const char *name) {
	const auto & value = getProperties().find(name);
	if (value != SymbolTable::Null) {
		return value;
	}
	if (class_definition) {
		const Value & result = getDefault(name);
		return result;
	}
	else {
		auto sc = findClass(getKind());
		if (sc) {
			const Value & result = sc->getDefaults().find(name);
			if (!isNull(result)) {
				return result;
			}
			else {
				return SymbolTable::Null;
			}
		}
		else {
			std::cerr << "no class: " << getKind() << "\n";
		}
	}
	std::cerr << "no class for structure: " << getName() << "\n";
	return SymbolTable::Null;
}

const std::map<std::string, std::string> &StructureClass::property_map() const {
	return m_property_map;
}
const std::map<std::string, std::string> &StructureClass::reverse_property_map() const {
	return m_reverse_map;
}


bool writePropertyList(std::ostream &out, const SymbolTable &properties, const SymbolTable *defaults, const std::map<std::string, std::string> * link_map) {
	const char *begin_properties = "(\n    ";
	const char *property_delim = ",\n    ";
	const char *delim = begin_properties;
	SymbolTableConstIterator i = properties.begin();
	while (i != properties.end()) {
		auto item = *i++;
		if (!isNull(item.second) && (link_map || item.second.asString() != "") ) {
			if (link_map) {
				auto remote_name = link_map->find(item.first);
				if (remote_name != link_map->end()) {
					out << delim << item.first << ": $" << Value( (*remote_name).second, Value::t_symbol);
					delim = property_delim;
					continue;
				}
			}
			// const Value &default_value = defaults ? defaults->find(item.first.c_str()) : SymbolTable::Null;
			// if (item.second == default_value) { continue; } // don't write unchanged properties
			if (item.first == "text" || item.first == "working_text") continue; // don't save text properties unless they were in a link map.
			if (item.second.kind == Value::t_string)
				out << delim << item.first << ": " << item.second; // quotes are automatically added to string values
			else
				out << delim << item.first << ": " << item.second;
			delim = property_delim;
		}
	}
	if (delim == property_delim) out << "\n  )"; // only output the ')' if there were properties
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

bool Structure::save(std::ostream &out, const std::string &structure_name) {
	out << name << " " << kind;
	std::map<std::string, std::string> link_map;
	auto links = LinkManager::instance().remote_links(structure_name, getName());
	if (links) {
		std::cout << "have " << links->size() << " links for " << getName() << " when writing property list" << "\n";
		if (links) {
			auto structure_class = findClass(kind);
			if (structure_class) {
				auto & reverse_map = structure_class->reverse_property_map();
				for (auto & link : *links) {
					auto prop = reverse_map.find(link.property_name);
					if (prop != reverse_map.end()) {
						std::cout << (*prop).first << " --> " << link.remote_name << "\n";
						link_map[(*prop).first] = link.remote_name;
					}
				}
			}
		}
	}
	bool res = writePropertyList(out, properties,
					getStructureDefinition() ? &getStructureDefinition()->getDefaults() : nullptr, &link_map);
	out << ";\n";
	return res;
}

bool StructureClass::save(std::ostream &out) {
	using namespace nanogui;
	out << getName() << " STRUCTURE";
	writePropertyList(out, properties, &defaults);
	if (!getBase().empty()) out << " EXTENDS " << getBase();
	out << " {\n";
	writeOptions(out, options);
	for (auto local : locals) {
		out << "  ";
		Structure *s = local.machine;
		s->save(out, getName());
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

void Structure::loadBuiltins() {
	StructureClass *sc = new StructureClass("BUTTON", "");
	sc->setBuiltIn();
	hm_classes.push_back(sc);
	sc = new StructureClass("FRAME", "");
	sc->setBuiltIn();
	hm_classes.push_back(sc);
	sc = new StructureClass("IMAGE", "");
	sc->setBuiltIn();
	hm_classes.push_back(sc);
	sc = new StructureClass("INDICATOR", "");
	sc->setBuiltIn();
	hm_classes.push_back(sc);
	sc = new StructureClass("LABEL", "");
	sc->setBuiltIn();
	hm_classes.push_back(sc);
	sc = new StructureClass("PLOT", "");
	sc->setBuiltIn();
	hm_classes.push_back(sc);
	sc = new StructureClass("PROGRESS", "");
	sc->setBuiltIn();
	hm_classes.push_back(sc);
	sc = new StructureClass("COMBOBOX", "");
	sc->setBuiltIn();
	hm_classes.push_back(sc);
	sc = new StructureClass("SCREEN", "");
	sc->setBuiltIn();
	hm_classes.push_back(sc);
	sc = new StructureClass("TEXT", "");
	sc->setBuiltIn();
	hm_classes.push_back(sc);
	const std::string keypad = R"()";
}

bool StructureClass::isExtension(const std::string & seek) {
	if (name == seek) { return true; }
	std::string check = base;
	while (!check.empty()) {
		if (check == seek) { return true; }
		auto found = findClass(check);
		if (!found) break;
		check = found->base;
	}
	return false;
}

void loadProperties(SymbolTable &properties);

// Structures are added to the list so extensions always follow base classes.
// Structures can arrive in any order.
void addStructureClass(StructureClass *new_class, std::list<StructureClass*> &classes) {
	if (new_class->getBase().empty()) {
		classes.push_front(new_class);
	}
	else {
		auto iter = classes.begin();
		while (iter != classes.end()) {
			const auto & sc  = *iter;
			if (sc->getName() == new_class->getBase()) {
				classes.insert(++iter, new_class);
				return;
			}
			else if (sc->getBase() == new_class->getBase() || sc->getBase() == new_class->getName()) {
				classes.insert(iter++, new_class);
				return;
			}
			else {
				++iter;
			}
		}
		classes.push_back(new_class);
	}
}

// check structure classes and return a list of detected errors
std::list<std::string> checkStructureClasses() {
	std::list<std::string> errors;
	for (const auto sc : hm_classes) {
		if (!sc->getBase().empty()) {
			const auto found = findClass(sc->getBase());
			if (!found) {
				std::stringstream ss;
				ss << "Base structure: " << sc->getBase() << " not found for class: " << sc->getName();
				errors.push_back(ss.str());
			}
		}
	}
	return errors;
}

SymbolTable default_properties(const StructureClass *s) {
	SymbolTable props;
	if (s->getBase().empty()) { return props; }
	std::string base = s->getName();
	std::list<StructureClass *>inheritance;
	for (auto iter = hm_classes.rbegin(); iter != hm_classes.rend(); ++iter) {
		const auto & sc = *iter;
		if (sc->getName() == base) {
			inheritance.push_back(sc);
			base = sc->getBase();
			if (base.empty()) { break; }
		}
	}

	for (const auto sc : inheritance) {
		for (const auto & option : sc->getOptions()) {
			props.add(option.first, option.second, SymbolTable::ST_REPLACE);
		}
		props.add(sc->getProperties(), SymbolTable::ST_REPLACE);
	}
	return props;
}
