//
//  LinkableProperty.cpp
//  Project: humid
//
//	All rights reserved. Use of this source code is governed by the
//	3-clause BSD License in LICENSE.txt.

#include <nanogui/progressbar.h>
#include <nanogui/button.h>
#include "linkableobject.h"
#include "editorobject.h"
#include "editorwidget.h"
#include "editorbutton.h"
#include "editorprogressbar.h"
#include "linkableproperty.h"
#include "helper.h"

LinkableProperty::LinkableProperty(const std::string group, int object_type,
                const std::string &name, const std::string &addr_str,
                const std::string &dtype, int dsize)
: EditorObject(nullptr, name), group_name(group), kind(object_type), tag_name(name), address_str(addr_str), data_type_name(dtype),
    data_type(CircularBuffer::dataTypeFromString(dtype)), data_size(dsize) {
      if (addr_str.length() > 3) {
        char *rest = 0;
        modbus_address = (int)strtol(address_str.c_str()+2,&rest, 10);
      }
    }

LinkableProperty::~LinkableProperty() {
	if (links.size()) {
		std::cout << "deleting linkable property " << name << " with " << links.size() << " links\n";
	}
}

const std::string &LinkableProperty::group() const { return group_name; }

void LinkableProperty::setGroup(const std::string g) { group_name = g; }

const std::string &LinkableProperty::tagName() const { return tag_name; }

const std::string &LinkableProperty::typeName() const { return data_type_name; }

void LinkableProperty::setTypeName(const std::string s) { data_type_name = s; }

CircularBuffer::DataType LinkableProperty::dataType() const { return data_type; }

void LinkableProperty::setDataTypeStr(const std::string dtype) {
    data_type = CircularBuffer::dataTypeFromString(dtype);
    data_type_name = dtype;
}

int LinkableProperty::getKind() const { return kind; }

void LinkableProperty::setKind(int new_kind) { kind = new_kind; }

std::string LinkableProperty::addressStr() const { return address_str; }

void LinkableProperty::setAddressStr(const std::string s) {
    address_str = s;
    char *rest = 0;
    if (address_str.at(0) == '\'')
        modbus_address = (int)strtol(address_str.c_str()+2,&rest, 10);
    else
        modbus_address = (int)strtol(address_str.c_str()+1,&rest, 10);
}

void LinkableProperty::setAddressStr(int grp, int addr) {
    char buf[10];
    snprintf(buf,10, "'%d%04d", grp, addr);
    modbus_address = addr;
}

void LinkableProperty::setValue(const Value &v) {
	current = v;
	for (auto link : links)
		link->update(v);
}

void LinkableProperty::apply() {
	for (auto link : links) {
		link->update(current);
  }
}

Value & LinkableProperty::value() { return current; }

int LinkableProperty::dataSize() const { return data_size; }

void LinkableProperty::setDataSize(int new_size) { data_size = new_size; }

int LinkableProperty::address() const { return modbus_address; }

void LinkableProperty::setAddress(int a) { modbus_address = a; setAddressStr(address_group(), a); }

int LinkableProperty::address_group() const {
    if (address_str.at(0) == '\'') return address_str.at(1)-'0'; else return address_str.at(0)-'0';
}

void LinkableProperty::link(LinkableObject *lo) {
    links.remove(lo);
    links.push_back(lo);
    lo->update(value());
}

void LinkableProperty::unlink(EditorObject *w) {
    std::list<LinkableObject*>::iterator iter = links.begin();
    while (iter != links.end()) {
        LinkableObject *link = *iter;
        if (link->linked() == w) {
            iter = links.erase(iter);
            delete link;
        }
        else iter++;
    }
}

void LinkableProperty::save(std::ostream &out) const {
	out << "group:" << shortName(group_name) << ", "
		<< "kind:" << kind << ", "
		<< "address:" << address_str << ", "
		<< "type:" << data_type_name << ", "
		<< "size:" << data_size << ", "
		<< "remote:" << tag_name;
}
