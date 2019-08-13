//
//  LinkableProperty.h
//  Project: humid
//
//	All rights reserved. Use of this source code is governed by the
//	3-clause BSD License in LICENSE.txt.

#ifndef __LinkableProperty_h__
#define __LinkableProperty_h__

#include <ostream>
#include <string>
#include <list>
#include <lib_clockwork_client.hpp>
#include <humid/common/includes.hpp>
#include "editorobject.h"

class LinkableProperty : public EditorObject {
public:
	LinkableProperty(const std::string group, int object_type,
					const std::string &name, const std::string &addr_str,
					const std::string &dtype, int dsize);
	const std::string &group() const;
	void setGroup(const std::string g);
	const std::string &tagName() const;
	const std::string &typeName() const;
	void setTypeName(const std::string s);
	Humid::DataType dataType() const;
	void setDataTypeStr(const std::string dtype);
	int getKind() const;
	void setKind(int new_kind);
	std::string addressStr() const;
	void setAddressStr(const std::string s);
	void setAddressStr(int grp, int addr);
	void setValue(const Value &v);
	Value & value();
	int dataSize() const;
	void setDataSize(int new_size);
	int address() const;
	void setAddress(int a);
	int address_group() const;
	void link(LinkableObject *lo);
	void unlink(EditorObject*w);
	void save(std::ostream &out) const;
	void apply();
	std::ostream &operator<<(std::ostream &out) const;
private:
	std::string group_name;
	int kind;
	std::string tag_name;
	std::string address_str;
	std::string data_type_name;
	Humid::DataType data_type;
	int data_size;
	int modbus_address;
	Value current;
	std::list<LinkableObject*> links;
};

std::ostream& operator<<(std::ostream &out, const LinkableProperty &);

class PropertyLink {
public:
	PropertyLink(LinkableProperty *lp, const Value &val) : remote(lp), value(val) {}
	virtual void set(const Value &val) { value = val; /*(setter(val); */ }
	virtual const Value &get() const { return value; }
protected:
	LinkableProperty *remote;
	Value value;
	/*
	const std::function<void(const Value &)> &setter;
    const std::function<Value()> &getter;
	*/
};

#endif
