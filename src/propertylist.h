/*
	All rights reserved. Use of this source code is governed by the
	3-clause BSD License in LICENSE.txt.
*/

#ifndef latprocc_PropertyList_h
#define latprocc_PropertyList_h

#include <map>
#include <ostream>
#include <string>
#include <vector>

class PropertyList {
  public:
    PropertyList(const PropertyList &orig);
    PropertyList &operator=(const PropertyList &other);
    std::ostream &operator<<(std::ostream &out) const;
    bool operator==(const PropertyList &other);

  private:
    std::vector<std::pair<std::string, int>> property_list;
};

std::ostream &operator<<(std::ostream &out, const PropertyList &m);

#endif
