/*
	All rights reserved. Use of this source code is governed by the
	3-clause BSD License in LICENSE.txt.
*/

#ifndef __EditorGUI_h__
#define __EditorGUI_h__

#include <ostream>
#include <string>

class EditorGUI {
public:
    EditorGUI(const char *msg);
    EditorGUI(const EditorGUI &orig);
    EditorGUI &operator=(const EditorGUI &other);
    std::ostream &operator<<(std::ostream &out) const;
    bool operator==(const EditorGUI &other);
    
private:
    std::string text;
};

std::ostream &operator<<(std::ostream &out, const EditorGUI &m);

#endif
