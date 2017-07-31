//
//  helper.cpp
//  Project: humid
//
//	All rights reserved. Use of this source code is governed by the
//	3-clause BSD License in LICENSE.txt.

#include <iostream>
#include "helper.h"
#include "NamedObject.h"
#include "structure.h"
#include "Editor.h"
#include "ScreensWindow.h"

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
		else {
			std::cout << "Skipping " << s->getName() << " : " << s->getKind() << " looking for " << seek << "\n";
		}
	}
	std::cout << "Screen " << seek << " not found\n";
	std::cout << "structures:\n";
	for (auto s : hm_structures) {
		std::cout << s->getName() << " : " << s->getKind() << "\n";
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
	}
	return 0;
}

void createScreens() {
	for (auto sc : hm_classes) {
		if (sc->getBase() == "SCREEN") {
			if (! findStructureFromClass(sc->getName())) {
				sc->instantiate();
				std::cout << "Instantiated screen " << sc->getName() << "\n";
			}
		}
	}
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
	Structure *s = new Structure(scrn_name, sc_name);
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
	Structure *s = new Structure(s_name, sc_name);
	hm_structures.push_back(s);
	if (EDITOR->gui()->getScreensWindow())
		EDITOR->gui()->getScreensWindow()->update();
	return s;
}
