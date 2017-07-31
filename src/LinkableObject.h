//
//  LinkableObject.h
//  Project: humid
//
//	All rights reserved. Use of this source code is governed by the
//	3-clause BSD License in LICENSE.txt.

#ifndef __LinkableObject_h__
#define __LinkableObject_h__

#include <ostream>
#include <string>
#include <list>
#include "value.h"
#include "EditorObject.h"
#include "CircularBuffer.h"

class LinkableProperty;
class Structure;
class EditorWidget;
class EditorObject;

class Anchor {
public:
	virtual ~Anchor() { }
  virtual Value get() { return SymbolTable::Null; }
	virtual void set(Value) {}
};

class Link {
	public:
		Link(Anchor *s, Anchor *d) : source(s), dest(d) {}
		virtual ~Link() {}
		void update() const { dest->set(source->get()); }
		Anchor *source;
		Anchor *dest;
};

class RemoteAnchor : public Anchor {
	public:
    RemoteAnchor(LinkableProperty *other) : remote(other) {}
		virtual Value get() override ;
		virtual void set(Value v) override;

private:
  LinkableProperty *remote;
};

class StructurePropertyAnchor : public Anchor {
	public:
		StructurePropertyAnchor(std::string prop, Structure *other) : property_name(prop), endpoint(other) {}
		virtual Value get() override;
		virtual void set(Value v) override;
private:
	std::string property_name;
	Structure *endpoint;
};

class WidgetPropertyAnchor : public Anchor {
	public:
    WidgetPropertyAnchor(std::string prop, EditorWidget *other) : property_name(prop), ew(other) {}
		virtual Value get() override;
		virtual void set(Value v) override;

private:
  std::string property_name;
	EditorWidget *ew;
};

class LinkableObject {
public:
	virtual ~LinkableObject() ;
	LinkableObject(EditorObject *w);
	virtual void update(const Value &value);
	EditorObject *linked() { return widget; }
    std::ostream &operator<<(std::ostream &out) const;
protected:
	EditorObject *widget;
	std::string property;
};

std::ostream &operator<<(std::ostream &out, const LinkableObject &m);


class LinkableText : public LinkableObject {
	public:
		LinkableText(EditorObject *w);
		void update(const Value &value) override;
};

class LinkableNumber: public LinkableObject {
	public:
		LinkableNumber(EditorObject *w);
		void update(const Value &value) override;
};

class LinkableIndicator : public LinkableObject {
	public:
		LinkableIndicator(EditorObject *w);
		void update(const Value &value) override;
};

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
	CircularBuffer::DataType dataType() const;
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
private:
	std::string group_name;
	int kind;
	std::string tag_name;
	std::string address_str;
	std::string data_type_name;
	CircularBuffer::DataType data_type;
	int data_size;
	int modbus_address;
	Value current;
	std::list<LinkableObject*> links;
};


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
