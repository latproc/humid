//
//  EditorObject.h
//  Project: humid
//
//	All rights reserved. Use of this source code is governed by the
//	3-clause BSD License in LICENSE.txt.

#ifndef __EditorObject_h__
#define __EditorObject_h__

#include "namedobject.h"
#include <string>

class LinkableProperty;
class PropertyLink;


class EditorObject : public NamedObject {
	public:
		explicit EditorObject(NamedObject *owner) : NamedObject(owner, "untitled") {}
		EditorObject(NamedObject *owner, const std::string &name) : NamedObject(owner, name), changed_(true) {}
		EditorObject(NamedObject *owner, const char *name) : NamedObject(owner, name) {}
		~EditorObject() override = default;
		void setChanged(bool which) { changed_ = which; }
		bool changed() const { return changed_; }
        std::ostream &operator<<(std::ostream &out) const override;
	protected:
		bool changed_ = false;
};

std::ostream &operator<<(std::ostream &out, const EditorObject &m);

//
#endif
