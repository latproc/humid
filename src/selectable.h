/*
	All rights reserved. Use of this source code is governed by the
	3-clause BSD License in LICENSE.txt.
*/

#ifndef __Selectable_h__
#define __Selectable_h__

#include <nanogui/widget.h>
#include <ostream>
#include <string>

class SelectableWidget;
class SelectableButton;
class Palette;

class Selectable {
  public:
    Selectable(Palette *pal);
    Selectable(const Selectable &orig) = delete;
    Selectable &operator=(const Selectable &other);
    std::ostream &operator<<(std::ostream &out) const;
    bool operator==(const Selectable &other);

    virtual ~Selectable();

    bool isSelected();
    void select();
    void deselect();
    virtual void justSelected();
    virtual void justDeselected();

  protected:
    Palette *palette = nullptr;
    bool mSelected;
  private:

};

std::ostream &operator<<(std::ostream &out, const Selectable &m);

#endif
