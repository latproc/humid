//
//  NamedObject.h
//  Project: humid
//
//	All rights reserved. Use of this source code is governed by the
//	3-clause BSD License in LICENSE.txt.

#ifndef __NamedObject_h__
#define __NamedObject_h__

#include <ostream>
#include <string>
#include <map>

class NamedObject {
public:
    NamedObject(NamedObject *parent = 0);
    NamedObject(NamedObject *parent, const char *object_name);
    NamedObject(NamedObject *parent, const std::string &object_name);

    virtual ~NamedObject();
    void setName(const std::string new_name);
    std::string fullName() const;
    std::string getName() const { return name; }

  	static std::string nextName(NamedObject*, const std::string prefix = "");
    static bool changeName(NamedObject *o, const std::string &oldname, const std::string &newname);
    std::map<std::string, NamedObject*> &siblings();

    void remove( const std::string &name);
    void add( const std::string &name, NamedObject *child);

    NamedObject *getParent() const { return parent; }
    static std::map<std::string, NamedObject*> &globals() { return global_objects; }
    bool named() { return _named; }

protected:
    bool _named;
    NamedObject *parent;
    std::map<std::string, NamedObject*>child_objects;
    std::string name;
	static std::map<std::string, NamedObject*>global_objects;
	static unsigned int user_object_sequence;
private:
   NamedObject(const NamedObject &orig);
   NamedObject &operator=(const NamedObject &other);
   std::ostream &operator<<(std::ostream &out) const;
   bool operator==(const NamedObject &other);
};

std::ostream &operator<<(std::ostream &out, const NamedObject &m);

//
#endif
