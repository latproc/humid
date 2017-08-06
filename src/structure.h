/*
	All rights reserved. Use of this source code is governed by the
	3-clause BSD License in LICENSE.txt.
*/
#ifndef __hmi_structure_h__
#define __hmi_structure_h__

#include "parameter.h"
#include <list>
#include <map>
#include <ostream>
#include <vector>
#include <symboltable.h>
#include "namedobject.h"
#include "viewlistcontroller.h"
#include "editorobject.h"
#include "linkableobject.h"
#include "linkableproperty.h"

class Structure;

class ClassExtension {
public:
	ClassExtension(const char *en) : name(en) { }
	const std::string getName() { return name; }
	std::vector<Parameter> &getParameters() { return parameters; }
private:
	std::string name;
	std::vector<Parameter> parameters;
};

bool writePropertyList(std::ostream &out, const SymbolTable &table);
bool writePropertyDefaults(std::ostream &out, const SymbolTable &table);

class StructureClass : public NamedObject {
public:
	StructureClass(const std::string class_name) : NamedObject(nullptr, class_name), builtin(false) {}
	StructureClass(const std::string class_name, const std::string base_class) : NamedObject(nullptr, class_name), base(base_class), builtin(false) {};
	std::map<std::string, Structure *> &getGlobalRefs() { return global_references; }
	SymbolTable &getProperties() { return properties; }
	void setProperties(const SymbolTable &other) { properties = other; }
	SymbolTable &getInternalProperties() { return internal_properties; }

	std::vector<Parameter> &getParameters() { return parameters; }
	std::map<std::string, Value> &getOptions() { return options; }
	std::vector<Parameter> &getLocals() { return locals; }
	void setDefinitionLocation(const std::string fnam, int lineno) {
		internal_properties.add("file_name", Value(fnam, Value::t_string));
	}

	void addLocal(Parameter item) { locals.push_back(item); }

	virtual void addProperty(const char *name);
	virtual void addProperty(const std::string &name);
	virtual void addPrivateProperty(const char *name);
	virtual void addPrivateProperty(const std::string &name);

	const std::string &getName() const { return name; }
	const std::string &getBase() const { return base; }

	virtual bool save(std::ostream &out);

	Structure *instantiate(Structure *parent);
	Structure *instantiate(Structure *parent,const std::string s_name);

	void setBuiltIn() { builtin = true; }
	bool isBuiltIn() { return builtin; }

protected:
	std::map<std::string, Structure *>global_references;
	std::string base;
	SymbolTable internal_properties;
	SymbolTable properties;
	std::vector<Parameter> parameters;
	std::vector<Parameter> locals;
	std::map<std::string, Value> options;
	std::set<std::string> local_properties;
	std::set<std::string> property_names;
	bool builtin;
};

class Structure : public NamedObject {
public:
	Structure(Structure *owner, const std::string sname, const std::string skind);
	virtual ~Structure() {}
	std::list<Parameter> parameters;
	void setStructureDefinition(StructureClass *sc) { class_definition = sc; }
	StructureClass *getStructureDefinition() { return class_definition; }
	// set the location of the instance of the structure (not its class)
	void setDefinitionLocation(const std::string fnam, int lineno) {
		internal_properties.add("file_name", Value(fnam, Value::t_string));
	}
	SymbolTable &getInternalProperties() { return internal_properties; }
	void setProperties(const SymbolTable &props) { properties.add(props); }
	SymbolTable &getProperties() { return properties; }
	int getIntProperty(const std::string name, int default_value = 0);

	const std::string &getName() { return name; }
	void setName( const std::string new_name) { name = new_name; }
	const std::string &getKind() { return kind; }

	//Structure *clone(const std::string new_name);

	std::ostream &operator<<(std::ostream &out) const;
	void setChanged( bool which) { changed_ = which; }
	bool changed() { return changed_; }

	void setOwner(Structure *other) { owner = other; }
	Structure *getOwner() { return owner; }

	virtual bool save(std::ostream &out);

protected:
	Structure(const Structure &other);
	SymbolTable internal_properties;
	StructureClass *class_definition;
	Structure *owner;
private:
	SymbolTable properties;
	std::string kind;
	bool changed_; // in memory copy has been modified
};
std::ostream &operator<<(std::ostream &out, const Structure &s);

#endif
