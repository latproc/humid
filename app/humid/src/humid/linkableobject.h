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
#include <lib_clockwork_client.hpp>
#include "lineplot/includes.hpp"
#include "editorobject.h"
#include "parameter.h"

class LinkableProperty;
class Structure;
class EditorWidget;
class EditorObject;

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

class LinkableVisibility : public LinkableObject {
	public:
		LinkableVisibility(EditorObject *w);
		void update(const Value &value) override;
};


#endif
