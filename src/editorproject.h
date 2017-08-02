//
//  EditorProject.h
//  Project: Humid
//
//	All rights reserved. Use of this source code is governed by the
//	3-clause BSD License in LICENSE.txt.

#ifndef __EditorProject_h__
#define __EditorProject_h__

#include <ostream>
#include <string>

/** EditorProject provides a way to manage project resources
*/

class EditorProject {
public:
    EditorProject(const char *basedir);
    EditorProject(const EditorProject &orig);
    EditorProject &operator=(const EditorProject &other);
    std::ostream &operator<<(std::ostream &out) const;
    bool operator==(const EditorProject &other);
    
private:
    std::string base_dir;
};

std::ostream &operator<<(std::ostream &out, const EditorProject &m);

//
#endif
