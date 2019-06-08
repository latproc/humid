//
//  EditorObject.h
//  Project: humid
//
//	All rights reserved. Use of this source code is governed by the
//	3-clause BSD License in LICENSE.txt.

#ifndef __EditorObject_h__
#define __EditorObject_h__

#include <ostream>
#include <string>
#include <value.h>
#include "namedobject.h"

class LinkableProperty;
class PropertyLink;


class EditorObject : public NamedObject {
	public:
		EditorObject(NamedObject *owner) {}
		EditorObject(NamedObject *owner, const std::string &name) : NamedObject(owner, name), changed_(true) {}
		EditorObject(NamedObject *owner, const char *name) : NamedObject(owner, name) {}
		virtual ~EditorObject() { }
		void setChanged(bool which) { changed_ = which; }
		bool changed() { return changed_; }
        std::ostream &operator<<(std::ostream &out) const;
	protected:
		bool changed_;
};

std::ostream &operator<<(std::ostream &out, const EditorObject &m);

//
#endif
