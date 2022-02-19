//
//  Connectable.h
//  Project: humid
//
//	All rights reserved. Use of this source code is governed by the
//	3-clause BSD License in LICENSE.txt.

#ifndef __Connectable_h__
#define __Connectable_h__

#include "linkableobject.h"
#include "linkableproperty.h"
#include <ostream>
#include <string>

/**
Connectable objects have a reference to other objects for the purpose
of updating values and sending events.
*/
class Connectable {
  public:
    Connectable(LinkableProperty *lp) : remote(lp) {}
    int address() const {
        if (remote)
            return remote->address();
        else
            return 0;
    }
    LinkableProperty *getRemote() { return remote; }
    void setRemote(LinkableProperty *lp) { remote = lp; }

  protected:
    LinkableProperty *remote;
};

#endif
