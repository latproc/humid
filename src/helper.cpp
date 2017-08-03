//
//  helper.cpp
//  Project: humid
//
//	All rights reserved. Use of this source code is governed by the
//	3-clause BSD License in LICENSE.txt.

#include <iostream>
#include "helper.h"
#include "namedobject.h"
#include "structure.h"
#include "editor.h"
#include "screenswindow.h"

extern std::list<Structure *>hm_structures;
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
		//else {
			//std::cout << "Skipping " << s->getName() << " : " << s->getKind() << " looking for " << seek << "\n";
		//}
	}
	/*
	std::cout << "Screen " << seek << " not found\n";
	std::cout << "structures:\n";
	for (auto s : hm_structures) {
		std::cout << s->getName() << " : " << s->getKind() << "\n";
	}
	*/
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
			std::cout << "Notice: structure " << s->getName() << " is now linked to class " << class_name << "\n";
			return s;
		}
	}
	return 0;
}

int createScreens() {
	int res = 0;
	for (auto sc : hm_classes) {
		if (sc->getBase() == "SCREEN") {
			if (! findStructureFromClass(sc->getName())) {
				sc->instantiate(nullptr);
				std::cout << "Instantiated screen " << sc->getName() << "\n";
				++res;
			}
		}
	}
	return res;
}


Structure *findStructure(const std::string &seek) {
	for (auto item : hm_structures ) {
		Structure *s = item;
		assert(s);
		if (s->getName() == seek) return s;
	}
	std::cout << "Structure " << seek << " not found\n";
	std::cout << "structures:\n";
	for (auto s : hm_structures) {
		std::cout << s->getName() << " : " << s->getKind() << "\n";
	}
	return 0;
}

Structure *createScreenStructure() {
	std::string sc_name("Screen_");
	sc_name += NamedObject::nextName(nullptr);
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
						collect_humid_files( fp / (*iter), files);
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