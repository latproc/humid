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

class LinkableObject {
public:
	virtual ~LinkableObject() ;
	LinkableObject();
	explicit LinkableObject(EditorObject *w);
	explicit LinkableObject(LinkTarget *target);
	virtual void update(const Value &value);
	EditorObject *linked() { return widget; }
    std::ostream &operator<<(std::ostream &out) const;
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
