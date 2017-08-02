//
//  Anchor.cpp
//  Project: humid
//
//	All rights reserved. Use of this source code is governed by the
//	3-clause BSD License in LICENSE.txt.

#include <iostream>
#include <string>
#include "Anchor.h"
#include "LinkableObject.h"
#include "LinkableProperty.h"
#include "Structure.h"
#include "EditorWidget.h"

std::ostream &Anchor::operator()(std::ostream &out) const {
  return out << "";
}
std::ostream &operator<<(std::ostream &out, const Anchor &anchor) {
  return anchor.operator()(out);
}

std::ostream &operator<<(std::ostream &out, const RemoteAnchor &ra) {
  return ra.operator()(out);
}
std::ostream &StructurePropertyAnchor::operator()(std::ostream &out) const {
  return out << "PROPERTY " << property_name << ";";
}
std::ostream &operator<<(std::ostream &out, const StructurePropertyAnchor &spa) {
  return spa.operator()(out);
}
std::ostream &StateAnchor::operator()(std::ostream &out) const {
  return out << structure_name << ";";
}
std::ostream &operator<<(std::ostream &out, const StateAnchor &sa) {
  return sa.operator()(out);
}
std::ostream &KeyAnchor::operator()(std::ostream &out) const {
  return out << "KEY " << key_code << ";";
}
std::ostream &operator<<(std::ostream &out, const KeyAnchor &ka) {
  return ka.operator()(out);
}
std::ostream &ActionAnchor::operator()(std::ostream &out) const {
  return out << "ACTION {}";
}
std::ostream &operator<<(std::ostream &out, const ActionAnchor &aa) {
  return aa.operator()(out);
}
std::ostream &WidgetPropertyAnchor::operator()(std::ostream &out) const {
  return out;
}
std::ostream &operator<<(std::ostream &out, const WidgetPropertyAnchor &wpa) {
  return wpa.operator()(out);
}




RemoteAnchor::~RemoteAnchor() {}
Value RemoteAnchor::get() { return remote->value(); }
void RemoteAnchor::set(Value v) { remote->setValue(v); }
std::ostream &RemoteAnchor::operator()(std::ostream &out) const {
  out << "REMOTE PROPERTY " << remote_name << ";";
  return out;
}

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

std::ostream &Link::operator()(std::ostream &out) const {
  return out << "LINK " << source << " TO " << dest << ";";
}

std::ostream &operator<<(std::ostream &out, const Link &link) {
  return link.operator()(out);
}
