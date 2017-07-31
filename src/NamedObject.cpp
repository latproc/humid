//
//  NamedObject.cpp
//  Project: humid
//
//	All rights reserved. Use of this source code is governed by the
//	3-clause BSD License in LICENSE.txt.

#include <assert.h>
#include <iostream>
#include <map>
#include "NamedObject.h"

std::map<std::string, NamedObject*>NamedObject::global_objects;
unsigned int NamedObject::user_object_sequence = 1;

NamedObject::NamedObject() : parent(0) {
    nextName(this);
}

NamedObject::~NamedObject() {
	if (!parent) {
    auto found = global_objects[name];
    if (found == this)
      global_objects.erase( name );
    else
      std::cout << "Error: global object " << name << " is not me\n";
  }
  else parent->remove(name);
}

NamedObject::NamedObject(const char *msg) : parent(0), name(msg) {
	if (!parent) {
		auto found = global_objects.find( name );
    if (found != global_objects.end()) {
      const std::string &found_name((*found).first);
      NamedObject *found_object = (*found).second;
      //assert(found == global_objects.end());
    }
		global_objects[name] = this;
	}
	else {
		parent->add(name, this);
	}
}

NamedObject::NamedObject(const std::string &msg) : parent(0), name(msg) {
  auto found = global_objects.find( name );
  if (found != global_objects.end() && (*found).second != this && (*found).second) {
		// strip rn_ prefixes in case this object was renamed previouslly..
		while (name.substr(0,3) == "rn_") name = name.substr(3);
		name = nextName(this, "rn");
		std::cout << "Error: " << msg << " already defined, renamed new object to " << name << "\n";
	}
	//assert(found == global_objects.end());
  global_objects[name] = this;
}

void NamedObject::add(const std::string &object_name, NamedObject *child) {
	auto found = child_objects.find( name );
	assert(found == child_objects.end());
	child_objects[name] = child;
}
void NamedObject::remove( const std::string &object_name) {
	child_objects.erase(name);
}

std::string NamedObject::fullName() {
	std::string parent_name;
	if (parent) {
		parent_name = parent->fullName();
		return parent_name + "." + name;
	}
	return name;
}

std::ostream &NamedObject::operator<<(std::ostream &out) const  {
    out << name;
    return out;
}

std::ostream &operator<<(std::ostream &out, const NamedObject &m) {
    return m.operator<<(out);
}

void NamedObject::setName(const std::string new_name) {
    if (NamedObject::changeName(this, name,new_name))
        name = new_name;
}


NamedObject::NamedObject(const NamedObject &orig){
    name = orig.name;
}
/*
NamedObject &NamedObject::operator=(const NamedObject &other) {
    name = other.name;
    return *this;
}
*/

bool NamedObject::operator==(const NamedObject &other) {
    return name == other.name;
}

std::string NamedObject::nextName(NamedObject *o, const std::string prefix) {
	char buf[40];
	if (prefix.length())
		snprintf(buf, 40, "%s_Untitled_%03d", prefix.c_str(), user_object_sequence);
	else
		snprintf(buf, 40, "Untitled_%03d", user_object_sequence);

	if (!o) {
		while (global_objects.find(buf) != global_objects.end()) {
			snprintf(buf, 40, "Untitled_%03d", ++user_object_sequence);
		}
		return buf;
	}
	std::map<std::string, NamedObject*>&siblings = o->siblings();
	std::string result(buf);
	o->setName(result);
	siblings[result] = o;
	return result;
}

std::string NamedObject::newChildName(NamedObject *o, const std::string prefix) {
	char buf[40];
	if (prefix.length())
		snprintf(buf, 40, "%s_Untitled_%03d", prefix.c_str(), user_object_sequence);
	else
		snprintf(buf, 40, "Untitled_%03d", user_object_sequence);

	while (child_objects.find(buf) != child_objects.end()) {
		snprintf(buf, 40, "Untitled_%03d", ++user_object_sequence);
	}
	if (o) {
		std::string result(buf);
		o->setName(result);
		child_objects[result] = o;
		return result;
	}
	return buf;
}

std::map<std::string, NamedObject*> &NamedObject::siblings() {
	if (!parent) return global_objects;
	return parent->child_objects;
}

bool NamedObject::changeName(NamedObject *o, const std::string &oldname, const std::string &newname) {
	if (oldname == newname) return true;
	std::map<std::string, NamedObject*> &siblings(o->siblings());
	std::map<std::string, NamedObject*>::iterator found = siblings.find(newname);
	if (found != siblings.end()) return false;
	siblings[newname] = o;
	found = siblings.find(oldname);
	if (found != siblings.end()) siblings.erase(found);
	return true;
}
