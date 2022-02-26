/*
	All rights reserved. Use of this source code is governed by the
	3-clause BSD License in LICENSE.txt.
*/

#include "palette.h"
#include "selectable.h"
#include <algorithm>
#include <iostream>
#include <list>

Palette::Palette(PaletteType pt) : kind(pt) {}

bool Palette::hasSelections() const { return selections.size() > 0; }
void Palette::select(Selectable *w) {
    if (!w) { return; }
    if (kind == PT_SINGLE_SELECT)
        clearSelections(w);
    selections.insert(w);
}
void Palette::deselect(Selectable *w) { selections.erase(w); }

void Palette::clearSelections(Selectable *except) {
    if (!selections.empty()) {
        std::list<Selectable *> to_deselect;
        std::copy(selections.begin(), selections.end(), std::back_inserter(to_deselect));
        for (auto iter = to_deselect.begin(); iter != to_deselect.end(); ++iter) {
            Selectable *sel = *iter;
            if (sel != except && sel->isSelected())
                sel->deselect();
        }
    }
}

const std::set<Selectable *> &Palette::getSelected() const { return selections; }

#if 0
Palette::Palette(const Palette &orig){
    text = orig.text;
}

Palette &Palette::operator=(const Palette &other) {
    text = other.text;
    return *this;
}

std::ostream &Palette::operator<<(std::ostream &out) const  {
    out << text;
    return out;
}

std::ostream &operator<<(std::ostream &out, const Palette &m) {
    return m.operator<<(out);
}

bool Palette::operator==(const Palette &other) {
    return text == other.text;
}
#endif
