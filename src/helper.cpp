//
//  helper.cpp
//  Project: humid
//
//	All rights reserved. Use of this source code is governed by the
//	3-clause BSD License in LICENSE.txt.

#include <iostream>
#include <boost/algorithm/string.hpp>

#include "helper.h"
#include "namedobject.h"
#include "structure.h"
#include "editor.h"
#include "screenswindow.h"
#include "circularbuffer.h"

extern std::list<Structure *>hm_structures;
extern std::list<Structure *>builtin_structures;

extern std::list<StructureClass *> hm_classes;

static int screen_id = 0;


std::string stripEscapes(const std::string &s) {
	size_t n = s.length()+1;
	char buf[n*2];
	char *p = buf;
	char last = 0;
	for (auto c : s) {
		if (last==0) { if (c == '"') continue; last=c; continue;}
		if (p+n*2-buf <= 2) break;
		if (last != '\\' || c != '"') *p++ = last;
		last = c;
	}
	*p++ = last;
	*p = 0;
	return buf;
}

std::string escapeQuotes(const std::string &s) {
	size_t n = s.length()+1;
	char buf[n*2];
	char *p = buf;
	char last = 0;
	bool need_quote = false;
	for (auto c : s) {
		if (p+n*2-buf <= 2) break;
		if (last == 0 && c != '"') {*p++ = '"'; need_quote = true; }
		if (last != '\\' && c == '"') *p++ = '\\';
		*p++ = c;
		last = c;
	}
	if (need_quote) *p++ = '"';
	*p = 0;
	return buf;
}


std::string stripChar(const std::string &s, char ch) {
	size_t n = s.length()+1;
	char buf[n*2];
	char *p = buf;
	for (auto c : s) {
		if (c != ch) *p++ = c;
	}
	*p = 0;
	return buf;
}

std::string shortName(const std::string s) {
	size_t pos = s.rfind("/");
	std::string n;
	if (pos != std::string::npos) n = s.substr(++pos); else n = s;
	pos = n.rfind('.');
	if (pos != std::string::npos) n.erase(pos);
	n = stripChar(n, '-');
	n = stripChar(n, '.');
	return n;
}

std::string extn(const std::string s) {
	size_t pos = s.rfind(".");
	if (pos != std::string::npos) return s.substr(pos+1);
	return s;
}

StructureClass *findClass(const std::string &name) {
		StructureClass *sc = nullptr;
		for (auto item : hm_classes) {
			if (item->getName() == name) return item;
		}
		return nullptr;
}

Structure *firstScreen() {
	for (auto item : hm_structures ) {
		Structure *s = item;
		assert(s);
		if (s->getStructureDefinition()
					&& (s->getStructureDefinition()->getName() == "SCREEN" || s->getStructureDefinition()->getBase() == "SCREEN") )
				return s;
	}
	return 0;
}

Structure *findScreen(const std::string &seek) {
	for (auto item : hm_structures ) {
		Structure *s = item;
		assert(s);
		StructureClass *sc = s->getStructureDefinition();
		if (!sc) {
			sc = findClass(s->getKind());
			if (sc) s->setStructureDefinition(sc);
		}
		if (s->getName() == seek && s->getStructureDefinition()
					&& (s->getStructureDefinition()->getName() == "SCREEN" || s->getStructureDefinition()->getBase() == "SCREEN") )
				return s;
	}
	return 0;
}

Structure * findStructureFromClass(std::string class_name) {
	for (auto item : hm_structures ) {
		Structure *s = item;
		assert(s);
		if (s->getStructureDefinition()
					&& (s->getStructureDefinition()->getName() == class_name
						|| s->getStructureDefinition()->getBase() == class_name) )
				return s;
		else if (!s->getStructureDefinition() && s->getKind() == class_name) {
			s->setStructureDefinition(findClass(class_name));
			return s;
		}
	}
	return 0;
}

Structure *findStructure(const std::string &seek, const std::list<Structure*> & library) {
	for (auto item : hm_structures ) {
		if (item->getName() == seek) return item;
	}
	return 0;
}

Structure *findStructure(const std::string &seek) {
	auto found = findStructure(seek, hm_structures);
	if (!found) found = findStructure(seek, builtin_structures);
	return found;
}

int createScreens() {
	int res = 0;
	for (auto sc : hm_classes) {
		if (sc->getBase() == "SCREEN") {
			if (! findStructureFromClass(sc->getName())) {
				sc->instantiate(nullptr);
				++res;
			}
		}
	}
	return res;
}

Structure *createScreenStructure() {
	std::string sc_name("Screen");
	sc_name = NamedObject::nextName(nullptr, sc_name);
	StructureClass *sc = new StructureClass(sc_name, "SCREEN");
	hm_classes.push_back(sc);
	std::string scrn_name(NamedObject::nextName(nullptr));
	Structure *s = new Structure(nullptr, scrn_name, sc_name);
	s->getProperties().add("screen_id", ++screen_id);
	hm_structures.push_back(s);
	if (EDITOR->gui()->getScreensWindow())
		EDITOR->gui()->getScreensWindow()->update();
	return s;
}

StructureClass *createStructureClass(const std::string kind) {
	StructureClass *sc = new StructureClass(kind, "");
	hm_classes.push_back(sc);
	return sc;
}

StructureClass *extendStructureClass(const std::string base) {
	std::string sc_name = NamedObject::nextName(nullptr);
	StructureClass *sc = new StructureClass(sc_name, base);
	hm_classes.push_back(sc);
	return sc;
}

Structure *createStructure(const std::string sc_name) {
	std::string s_name(NamedObject::nextName(nullptr));
	Structure *s = new Structure(nullptr, s_name, sc_name);
	hm_structures.push_back(s);
	if (EDITOR->gui()->getScreensWindow())
		EDITOR->gui()->getScreensWindow()->update();
	return s;
}

void collect_humid_files(boost::filesystem::path fp, std::list<boost::filesystem::path> &files) {
		using namespace boost::filesystem;
		if (!is_directory(fp)) return;
		typedef std::vector<path> path_vec;
		path_vec items;
		std::copy(directory_iterator(fp), directory_iterator(), std::back_inserter(items));
		std::sort(items.begin(), items.end());
		for (path_vec::const_iterator iter(items.begin()); iter != items.end(); ++iter) {
				if (is_regular_file(*iter) ) {
						path fn(*iter);
						std::string ext = boost::filesystem::extension(fn);
						if (ext == ".humid") files.push_back(fn);
				}
				else if (is_directory(fp)) {
						collect_humid_files( (*iter), files);
				}
		}
}

void backup_humid_files(boost::filesystem::path base) {
		using namespace boost::filesystem;

		std::list<path> all_humid_files;

		collect_humid_files(base, all_humid_files);

		// backup all .humid files
		for (auto item : all_humid_files) {
			std::cout << "backing up humid file: " << item.string();
			if (!exists(item)) {
				std::cout << " aborted: file does not exist\n";
				continue;
			}
			path backup(item);
			backup += '_';
			std::cout << " to " << backup << "\n";
			boost::filesystem::rename(item,backup);
		}
}

nanogui::Color colourFromProperty(Structure *element, const std::string &prop) {
	return colourFromProperty(element, prop.c_str());
}

nanogui::Color colourFromProperty(Structure *element, const char *prop) {
	const Value &bg_colour(element->getProperties().find(prop));
	if (bg_colour != SymbolTable::Null) {
		std::vector<std::string> tokens;
		std::string colour_str = bg_colour.asString();
		boost::algorithm::split(tokens, colour_str, boost::is_any_of(","));
		if (tokens.size() == 4) {
			std::vector<float>fields(4);
			for (int i=0; i<4; ++i) fields[i] = std::atof(tokens[i].c_str());
			return nanogui::Color(fields[0], fields[1], fields[2], fields[3]);
		}
	}
	return nanogui::Color(0.0f, 0.0f, 0.0f, 1.0f);
}

int dataTypeFromModbus(int val, int len) {
	return 0;
#if 0
	enum DataType { INT16, UINT16, INT32, UINT32, DOUBLE, STR };
		if (val == 3) return INT16; // readonly
		else if (val == 4) return INT16; // readwrite
		else if (val == 5) return INT32;
		else if (val == 4 && len == 2) return INT32;
		else if (val == 6 && len == 2) return INT32;
		else if (val == 8) return DOUBLE;
		else if (s == "Ascii_string") return STR;
		else if (s == "Floating_PT_32") return DOUBLE;
		else if (s == "Discrete") return INT16;
		return DOUBLE;
#endif
}

void getPropertyNames(std::list<std::string> &names) {
  names.push_back("Border");
  names.push_back("Connection");
  names.push_back("Font Size");
  names.push_back("Format");
  names.push_back("Height");
  names.push_back("Horizontal Pos");
  names.push_back("Inverted Visibility");
  names.push_back("Name"); // not common
  names.push_back("Remote");
  names.push_back("Structure");
  names.push_back("Tab Position");
  names.push_back("Value Scale");
  names.push_back("Value Type");
  names.push_back("Vertical Pos");
  names.push_back("Visibility");
  names.push_back("Width");
}

void loadPropertyToStructureMap(std::map<std::string, std::string> &property_map) {
  property_map["Border"] = "border";
  property_map["Connection"] = "connection";
  property_map["Font Size"] = "font_size";
  property_map["Format"] = "format";
  property_map["Height"] = "height";
  property_map["Horizontal Pos"] = "pos_x";
  property_map["Inverted Visibility"] = "inverted_visibility";
  property_map["Remote"] = "remote";
  property_map["Structure"] = ""; // not to be copied
  property_map["Tab Position"] = "tab_position";
  property_map["Value Scale"] = "value_scale";
  property_map["Value Type"] = "value_type";
  property_map["Vertical Pos"] = "pos_y";
  property_map["Visibility"] = "visibility";
  property_map["Width"] = "width";
}

