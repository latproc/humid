//
//  Anchor.h
//  Project: humid
//
//	All rights reserved. Use of this source code is governed by the
//	3-clause BSD License in LICENSE.txt.

#ifndef __Anchor_h__
#define __Anchor_h__

#include <ostream>
#include <string>
#include <list>
#include "value.h"
#include "Parameter.h"

class Structure;
class EditorWidget;
class LinkableProperty;

class Command {
public:
	Command() {}
	virtual ~Command() {}
};

class Action {
public:
	Action(std::string name, std::list<Parameter> &params, std::list<Command *> &commands) {}
protected:
	std::list<Command *>steps;
};

class Anchor {
public:
	virtual ~Anchor() { }
  virtual Value get() { return SymbolTable::Null; }
	virtual void set(Value) { }
  virtual std::ostream &operator()(std::ostream &out) const;
};
std::ostream &operator<<(std::ostream &out, const Anchor &);

class Link {
	public:
		Link(Anchor *s, Anchor *d) : source(s), dest(d) {}
		virtual ~Link() {}
		void update() const { dest->set(source->get()); }
    std::ostream &operator()(std::ostream &out) const;
		Anchor *source;
		Anchor *dest;
};
std::ostream &operator<<(std::ostream &out, const Link &);

class RemoteAnchor : public Anchor {
	public:
    RemoteAnchor(std::string name) : remote_name(name), remote(0) {}
		virtual ~RemoteAnchor();
		virtual Value get() override;
		virtual void set(Value v) override;
    virtual std::ostream &operator()(std::ostream &out) const override;

private:
  std::string remote_name;
  LinkableProperty *remote;
};
std::ostream &operator<<(std::ostream &out, const RemoteAnchor &);

class StructurePropertyAnchor : public Anchor {
	public:
		StructurePropertyAnchor(std::string prop, Structure *other) : property_name(prop), endpoint(other) {}
		virtual ~StructurePropertyAnchor() {}
		virtual Value get() override;
		virtual void set(Value v) override;
    virtual std::ostream &operator()(std::ostream &out) const override;
private:
	std::string property_name;
	Structure *endpoint;
};
std::ostream &operator<<(std::ostream &out, const StructurePropertyAnchor &);

class StateAnchor : public Anchor {
	public:
		StateAnchor(std::string prop, Structure *other) : structure_name(prop), endpoint(other) {}
		virtual ~StateAnchor() {}
		virtual Value get() override { return SymbolTable::Null; }
		virtual void set(Value v) override {}
    virtual std::ostream &operator()(std::ostream &out) const override;
	std::string structure_name;
	Structure *endpoint;
};
std::ostream &operator<<(std::ostream &out, const StateAnchor &);

class KeyAnchor : public Anchor {
	public:
		KeyAnchor(int key) : key_code(key) {}
		virtual ~KeyAnchor() {}
		virtual Value get() override { return SymbolTable::Null; }
		virtual void set(Value v) override {};
    virtual std::ostream &operator()(std::ostream &out) const override;
private:
	int key_code;
};
std::ostream &operator<<(std::ostream &out, const KeyAnchor &);

class ActionAnchor : public Anchor {
	public:
		ActionAnchor(Action*a) : action(a) {}
		virtual ~ActionAnchor() {}
		virtual Value get() override{ return SymbolTable::Null; }
		virtual void set(Value v) override {};
    virtual std::ostream &operator()(std::ostream &out) const override;
private:
	Action *action;
};
std::ostream &operator<<(std::ostream &out, const ActionAnchor &);

class WidgetPropertyAnchor : public Anchor {
	public:
    WidgetPropertyAnchor(std::string prop, EditorWidget *other) : property_name(prop), ew(other) {}
		virtual ~WidgetPropertyAnchor() {}
		virtual Value get() override;
		virtual void set(Value v) override;
    virtual std::ostream &operator()(std::ostream &out) const override;

private:
  std::string property_name;
	EditorWidget *ew;
};
std::ostream &operator<<(std::ostream &out, const WidgetPropertyAnchor &);

//
#endif
