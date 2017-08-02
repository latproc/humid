/*
	All rights reserved. Use of this source code is governed by the
	3-clause BSD License in LICENSE.txt.
*/

#ifndef __UIItem_h__
#define __UIItem_h__

#include <ostream>
#include <string>

class UIItem {
public:
	UIItem(const std::string kind) : item_class(kind) { }
	const std::string &getClass() const { return item_class; }

	UIItem(const UIItem &orig);
	UIItem &operator=(const UIItem &other);
	std::ostream &operator<<(std::ostream &out) const;
	bool operator==(const UIItem &other);

private:
	std::string item_class;
};

std::ostream &operator<<(std::ostream &out, const UIItem &m);

#endif
