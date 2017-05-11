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
    Palette();
    Palette(const Palette &orig);
    Palette &operator=(const Palette &other);
    std::ostream &operator<<(std::ostream &out) const;
    bool operator==(const Palette &other);
    
	bool hasSelections() const;
	void select(Selectable * w);
	void deselect(Selectable *w);
	void clearSelections();

	const std::set<Selectable *> &getSelected() const;
protected:
	std::set<Selectable *>selections;
};

std::ostream &operator<<(std::ostream &out, const Palette &m);

#endif
