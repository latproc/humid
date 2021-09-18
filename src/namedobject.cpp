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
    setName(nextName(this));
}

NamedObject::~NamedObject() {
  std::cout << "removing " << name << "\n";
	if (!parent) {
    auto found = global_objects.find(name);
    if (found != global_objects.end()) {
      assert((*found).second == this);
      global_objects.erase( found );
    }
    else
      std::cerr << "Error: global object " << name << " was never registered\n";
  }
  else parent->remove(name);
}

void addNamedObjectToMap(NamedObject *no, Dict &dict) {
  if (!no) return;

  auto found = dict.find(no->getName());
  if (found != dict.end() && (*found).second != no) {
    std::cerr << "object " << no->getName() << " already exists\n";
  }
  //assert(found == dict.end() || (*found).second == no);
  dict[no->getName()] = no;
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
	child_objects.erase(object_name);
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

std::string makeName(int id, const std::string &prefix) {
  char buf[40];
  if (prefix.length())
    snprintf(buf, 40, "%s_Untitled_%03d", prefix.c_str(), id);
  else
    snprintf(buf, 40, "Untitled_%03d", id);
  return buf;
}

std::string NamedObject::nextName(NamedObject *o, const std::string prefix) {
  Dict &dict( (o) ? o->locals() : global_objects);
  std::string trial_name = makeName(++user_object_sequence, prefix);
  auto item = dict.find(trial_name);
	while ( item != dict.end()) {
    if ((*item).second == o) return trial_name; // this name is already assigned to the object
    trial_name = makeName(++user_object_sequence, prefix);
    item = dict.find(trial_name);
	}
	return trial_name;
}

void NamedObject::forgetGlobal(const std::string &global_name) {
  auto found = global_objects.find(global_name);
  if (found != global_objects.end()) {
    global_objects.erase(found);
  }  
}

void NamedObject::setParent(NamedObject *o) {
  assert(parent == nullptr);
  auto found = global_objects.find(name);
  if (found != global_objects.end()) {
    global_objects.erase(found);
  }
  parent = o;
  parent->add(name, this);
}

std::map<std::string, NamedObject*> &NamedObject::siblings() {
	if (!parent) return global_objects;
  assert(& parent->child_objects != &global_objects);
	return parent->child_objects;
}

std::map<std::string, NamedObject*> &NamedObject::locals() {
  return child_objects;
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
