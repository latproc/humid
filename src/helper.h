//
//  helper.h
//  Project: humid
//
//	All rights reserved. Use of this source code is governed by the
//	3-clause BSD License in LICENSE.txt.

#ifndef __helper_h__
#define __helper_h__

#include <ostream>
#include <string>
#include <boost/filesystem.hpp>
#include <nanogui/common.h>
#include <map>
#include <value.h>

class StructureClass;
class Structure;

std::string stripEscapes(const std::string &s);
std::string escapeQuotes(const std::string &s);

std::string stripChar(const std::string &s, char ch);
std::string shortName(const std::string s);
std::string extn(const std::string s);

StructureClass *findClass(const std::string &name);
Structure *firstScreen();
Structure *findScreen(const std::string &seek);
Structure *findStructure(const std::string &seek);
Structure *findStructureFromClass(std::string class_name);
Structure *createScreenStructure();
StructureClass *createStructureClass(const std::string kind);
StructureClass *extendStructureClass(const std::string base);
Structure *createStructure(const std::string base);
int createScreens();

void collect_humid_files(boost::filesystem::path fp, std::list<boost::filesystem::path> &files);
void backup_humid_files(boost::filesystem::path base);
nanogui::Color colourFromProperty(Structure *s, const std::string &prop);
nanogui::Color colourFromProperty(Structure *element, const char *prop);
int dataTypeFromModbus(int val);
void getPropertyNames(std::list<std::string> &names);
void loadPropertyToStructureMap(std::map<std::string, std::string> &property_map);
std::ostream & displaySize(std::ostream &out, const std::string context, const nanogui::Vector2i s);
void invert_map(const std::map<std::string, std::string> &normal, std::map<std::string, std::string> & reversed);
#endif
