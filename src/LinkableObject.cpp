//
//  LinkableObject.cpp
//  Project: humid
//
//	All rights reserved. Use of this source code is governed by the
//	3-clause BSD License in LICENSE.txt.

#include <iostream>
#include <nanogui/progressbar.h>
#include <nanogui/button.h>
#include "LinkableObject.h"
#include "EditorObject.h"
#include "EditorWidget.h"
#include "EditorButton.h"
#include "EditorProgressBar.h"

extern std::string shortName(const std::string s);


Value RemoteAnchor::get() { return remote->value(); }
void RemoteAnchor::set(Value v) { remote->setValue(v); }


Value StructurePropertyAnchor::get() {
  return endpoint->getProperties().find( property_name.c_str() );
}

void StructurePropertyAnchor::set(Value v) {
  endpoint->getProperties().add(property_name.c_str(), v);
}

Value WidgetPropertyAnchor::get() {
  return ew->getPropertyValue(property_name);
}

void WidgetPropertyAnchor::set(Value v) {
  ew->setPropertyValue(property_name, v);
}

std::ostream &LinkableObject::operator<<(std::ostream &out) const {
    return out;
}
std::ostream &operator<<(std::ostream &out, const LinkableObject &m) {
    return m.operator<<(out);
}

void LinkableObject::update(const Value &v) {
  assert(false);
}

LinkableObject::~LinkableObject() {
    nanogui::Widget *w = dynamic_cast<nanogui::Widget *>(widget);
    if (w) w->decRef();
}

LinkableObject::LinkableObject(EditorObject *ew) : widget(ew) {
    nanogui::Widget *w = dynamic_cast<nanogui::Widget *>(widget);
    if (w) w->incRef();
}

LinkableText::LinkableText(EditorObject *w) : LinkableObject(w) { }

LinkableNumber::LinkableNumber(EditorObject *w) : LinkableObject(w) { }
void LinkableNumber::update(const Value &value) {
    EditorProgressBar *pb = dynamic_cast<EditorProgressBar*>(widget);
    if (pb) {
        if (value.kind == Value::t_integer)
            pb->setValue(value.iValue);
        else if (value.kind == Value::t_float)
            pb->setValue(value.fValue);
    }
}

LinkableIndicator::LinkableIndicator(EditorObject *w) : LinkableObject(w) { }
void LinkableIndicator::update(const Value &value) {
    EditorButton *tb = dynamic_cast<EditorButton*>(widget);
    if (tb) {
        if ( value.kind == Value::t_bool ) {
            tb->setPushed(value.bValue);
        }
        else if (value.kind == Value::t_integer) {
            tb->setPushed(value.iValue == 0);
        }
    }
}


LinkableProperty::LinkableProperty(const std::string group, int object_type,
                const std::string &name, const std::string &addr_str,
                const std::string &dtype, int dsize)
: EditorObject(name), group_name(group), kind(object_type), tag_name(name), address_str(addr_str), data_type_name(dtype),
    data_type(CircularBuffer::dataTypeFromString(dtype)), data_size(dsize) {
        char *rest = 0;
        modbus_address = (int)strtol(address_str.c_str()+2,&rest, 10);
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

Value & LinkableProperty::value() { return current; }
int LinkableProperty::dataSize() const { return data_size; }
void LinkableProperty::setDataSize(int new_size) { data_size = new_size; }
int LinkableProperty::address() const { return modbus_address; }
void LinkableProperty::setAddress(int a) { modbus_address = a; setAddressStr(address_group(), a); }
int LinkableProperty::address_group() const {
    if (address_str.at(0) == '\'') return address_str.at(1)-'0'; else return address_str.at(0)-'0';
}

void LinkableProperty::link(LinkableObject *lo) {
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
