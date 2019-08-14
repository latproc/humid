/*
	All rights reserved. Use of this source code is governed by the
	3-clause BSD License in LICENSE.txt.
*/

#ifndef __Palette_h__
#define __Palette_h__

#include <ostream>
#include <string>
#include <set>

class Selectable;
class Palette {
public:
	enum PaletteType { PT_MULTIPLE_SELECT, PT_SINGLE_SELECT };

    Palette(PaletteType pt = PT_MULTIPLE_SELECT);
    Palette(const Palette &orig);
    Palette &operator=(const Palette &other);
    std::ostream &operator<<(std::ostream &out) const;
    bool operator==(const Palette &other);
    
	bool hasSelections() const;
	PaletteType getType() { return kind; }
	virtual void select(Selectable * w);
	virtual void deselect(Selectable *w);
	virtual void clearSelections(Selectable * except = 0);
	virtual void reset(); // clear selections without attempting callbacks

	const std::set<Selectable *> &getSelected() const;
protected:
	PaletteType kind;
	std::set<Selectable *>selections;
};

std::ostream &operator<<(std::ostream &out, const Palette &m);

#endif
