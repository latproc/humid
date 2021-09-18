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
#include "editorobject.h"
#include "circularbuffer.h"
#include "parameter.h"

class LinkableProperty;
class Structure;
class EditorWidget;
class EditorObject;

class LinkTarget {
public:
	virtual void update(const Value &value) = 0;
	virtual ~LinkTarget() = default;
};

class PropertyLinkTarget : public LinkTarget {
public:
	PropertyLinkTarget(EditorWidget *widget, const std::string & property, const Value &default_value);
	void update(const Value &value);
	virtual ~PropertyLinkTarget();
	EditorWidget *widget() { return widget_; }
private:
    EditorWidget *widget_ = nullptr;
	std::string property_name;
	Value default_value;
};

class LinkableObject {
public:
	virtual ~LinkableObject() ;
	LinkableObject();
	explicit LinkableObject(EditorObject *w);
	explicit LinkableObject(LinkTarget *target);
	virtual void update(const Value &value);
	EditorObject *linked();
    std::ostream &operator<<(std::ostream &out) const;
	static void unlink(const std::string &class_name, EditorWidget *);
protected:
	EditorObject *widget = nullptr;
	LinkTarget *target = nullptr;
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

class LinkableVisibility : public LinkableObject {
	public:
		LinkableVisibility(EditorObject *w);
		void update(const Value &value) override;
};


#endif
