//
//  NamedObject.cpp
//  Project: humid
//
//	All rights reserved. Use of this source code is governed by the
//	3-clause BSD License in LICENSE.txt.

#include <assert.h>
#include <iostream>
#include <map>
#include "namedobject.h"

typedef std::map<std::string, NamedObject*> Dict;
Dict NamedObject::global_objects;
unsigned int NamedObject::user_object_sequence = 1;

NamedObject::NamedObject(NamedObject *owner) : _named(false), parent(owner) {
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

void addNamedObjectToMap(NamedObject *no, Dict &dict) {
  if (!no) return;
  dict[no->getName()] = no;
  /*
  //Dict &dict( (no) ? no->siblings() : NamedObject::globals());
  auto found = dict.find(no->getName());
  if (found != dict.end()) {
    NamedObject *other = (*found).second;
    if (!other || other == no) {
      (*found).second = no;
    }
    else {
      std::string name = no->getName();
      while (name.substr(0,3) == "rn_") name = name.substr(3);
      name = NamedObject::nextName(no, "rn");
      std::cout << "Warning: named " << no->getName()
        << " was already allocated to a different object; "
        << " renamed this one to " << name << "\n";
      no->setName(name);
    }
  }
  else
    dict[no->getName()] = no;
    */
}

NamedObject::NamedObject(NamedObject *owner, const char *msg) : _named(false), parent(owner), name(msg) {
	if (!parent)
    addNamedObjectToMap(this, global_objects);
	else
		parent->add(name, this);
}

NamedObject::NamedObject(NamedObject *owner, const std::string &msg) : _named(false), parent(owner), name(msg) {
  if (!parent)
    addNamedObjectToMap(this, global_objects);
	else
		parent->add(name, this);
}

void NamedObject::add(const std::string &object_name, NamedObject *child) {
  addNamedObjectToMap(child, child_objects);
}
void NamedObject::remove( const std::string &object_name) {
	child_objects.erase(name);
}

std::string NamedObject::fullName() const {
	std::string parent_name;
	if (parent) {
		parent_name = parent->fullName();
		return parent_name + "." + name;
	}
	return name;
}

std::ostream &NamedObject::operator<<(std::ostream &out) const  {
    out << fullName();
    return out;
}
/*
std::ostream &operator<<(std::ostream &out, const NamedObject &m) {
    return m.operator<<(out);
}
*/

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

/*bool NamedObject::operator==(const NamedObject &other) {
    return name == other.name;
}*/

std::string NamedObject::nextName(NamedObject *o, const std::string prefix) {
	char buf[40];
	if (prefix.length())
		snprintf(buf, 40, "%s_Untitled_%03d", prefix.c_str(), user_object_sequence);
	else
		snprintf(buf, 40, "Untitled_%03d", user_object_sequence);

  Dict &dict( (o) ? o->siblings() : global_objects);

  auto item = dict.find(buf);
	while ( item != dict.end()) {
    if ((*item).second == o) return buf; // this name is already assigned to the object
    if (prefix.length())
  		snprintf(buf, 40, "%s_Untitled_%03d", prefix.c_str(), ++user_object_sequence);
  	else
  		snprintf(buf, 40, "Untitled_%03d", ++user_object_sequence);
    item = dict.find(buf);
	}
  if (o)
    addNamedObjectToMap(o, dict);
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
