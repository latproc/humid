/*
	All rights reserved. Use of this source code is governed by the
	3-clause BSD License in LICENSE.txt.
*/
#ifndef __hmi_structure_h__
#define __hmi_structure_h__

#include "Parameter.h"
#include <list>
#include <map>
#include <ostream>
#include <vector>
#include <symboltable.h>

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

class StructureClass {
public:
	StructureClass(const std::string class_name) : name(class_name) {}
	StructureClass(const std::string class_name, const std::string base_class) : name(class_name), base(base_class) {};
	std::map<std::string, Structure *> &getGlobalRefs() { return global_references; }
	SymbolTable &getProperties() { return properties; }
	void setProperties(const SymbolTable &other) { properties = other; }
	std::vector<Parameter> &getParameters() { return parameters; }
	std::map<std::string, Value> &getOptions() { return options; }
	std::vector<Parameter> &getLocals() { return locals; }

	virtual void addProperty(const char *name);
	virtual void addProperty(const std::string &name);
	virtual void addPrivateProperty(const char *name);
	virtual void addPrivateProperty(const std::string &name); 

	const std::string &getName() const { return name; }
	const std::string &getBase() const { return base; }

private:
	std::map<std::string, Structure *>global_references;
	std::string name;
	std::string base;
	SymbolTable properties;
	std::vector<Parameter> parameters;
	std::vector<Parameter> locals;
	std::map<std::string, Value> options;
	std::set<std::string> local_properties;
	std::set<std::string> property_names;
};

class Structure {
public:
	Structure(const std::string sname, const std::string skind) : name(sname), kind(skind) {}
	std::list<Parameter> parameters;
	void setDefinitionLocation(const std::string fnam, int lineno) { }
	void setProperties(const SymbolTable &props) { properties.add(props); }
	SymbolTable &getProperties() { return properties; }
	int getIntProperty(const std::string name, int default_value = 0);

	const std::string &getName() { return name; }
	void setName( const std::string new_name) { name = new_name; }
	const std::string &getKind() { return kind; }

	Structure *clone(const std::string new_name);

	std::ostream &operator<<(std::ostream &out) const;
protected:
	Structure(const Structure &other);
private:
	SymbolTable properties;
	std::string name;
	std::string kind;
};
std::ostream &operator<<(std::ostream &out, const Structure &s);

#endif
