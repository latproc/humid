/*
	All rights reserved. Use of this source code is governed by the
	3-clause BSD License in LICENSE.txt.
*/
#include <string.h>
#include <stdlib.h>
#include <libgen.h>
#include <list>
#include <string>
#include <stdio.h>
#include <sstream>
#include <boost/utility.hpp>
#include <boost/filesystem.hpp>
#include <sys/stat.h>
#ifdef __APPLE__
#include <sys/dir.h>
#endif

#include <symboltable.h>
#define __MAIN__ 1
SymbolTable globals;
std::list<std::string>error_messages;
int num_errors = 0;

#include "hmilang.h"
extern std::list<Structure *>hm_structures;
extern std::list<StructureClass *> hm_classes;
extern FILE *yyin;
int yyparse();
extern int yycharno;
extern int yylineno;
const char *yyfilename = 0;
const char *filename = 0;
class LinkableProperty;
std::map<std::string, LinkableProperty *> remotes;

/* dummy routins */
void LinkableProperty::setValue(const Value &v) {}
Value & LinkableProperty::value() { return current; }


const char *program_name;

static void listDirectory( const std::string pathToCheck, std::list<std::string> &file_list)
{
    boost::filesystem::path dir(pathToCheck.c_str());
    try {
        for (boost::filesystem::directory_iterator iter = boost::filesystem::directory_iterator(dir); iter != boost::filesystem::directory_iterator(); iter++)
        {
            boost::filesystem::directory_entry file = *iter;
            char *path_str = strdup(file.path().native().c_str());
            struct stat file_stat;
            int err = stat(path_str, &file_stat);
            if (err == -1) {
                std::cerr << "Error: " << strerror(errno) << " checking file type for " << path_str << "\n";
            }
            else if (file_stat.st_mode & S_IFDIR){
                listDirectory(path_str, file_list);
            }
            else if (boost::filesystem::exists(file.path()) &&
				(file.path().extension() == ".lpc" || file.path().extension() == ".cw") )
            {
                file_list.push_back( file.path().native() );
            }
            free(path_str);
        }
    }
    catch (const boost::filesystem::filesystem_error& ex)
    {
        std::cerr << ex.what() << '\n';
    }
}

void loadFiles(std::list<std::string> &files) {

    /* load configuration from files named on the commandline */
    int opened_file = 0;
    std::list<std::string>::iterator f_iter = files.begin();
    while (f_iter != files.end())
    {
        const char *param = (*f_iter).c_str();
        if (param[0] != '-')
        {
            boost::filesystem::path path_fix(param);
            std::string filename = path_fix.string();
            opened_file = 1;
            yyin = fopen(filename, "r");
            if (yyin)
            {
                // std::cerr << "Processing file: " << filename << "\n";
                yylineno = 1;
                yycharno = 1;
                yyfilename = filename;
                yyparse();
                fclose(yyin);
            }
            else
            {
				std::stringstream ss;
                ss << "## - Error: failed to load config: " << filename;
				error_messages.push_back(ss.str());
                ++num_errors;
            }
        }
        else if (strlen(filename) == 1) /* '-' means stdin */
        {
            opened_file = 1;
            std::cerr << "\nProcessing stdin\n";
			yyfilename = "stdin";
            yyin = stdin;
            yylineno = 1;
            yycharno = 1;
            yyparse();
        }
        f_iter++;
    }
}


int main (int argc, char const *argv[])
{
	char *pn = strdup(argv[0]);
	program_name = strdup(basename(pn));
	free(pn);

	std::list<std::string> files;

	for (int i=1; i<argc; ++i) {
		if (*(argv[i]) == '-') {
			files.push_back(argv[i]);
		}
        else {
            struct stat file_stat;
            int err = stat(argv[i], &file_stat);
            if (err == -1) {
                std::cerr << "Error: " << strerror(errno) << " checking file type for " << argv[i] << "\n";
            }
            else if (file_stat.st_mode & S_IFDIR){
                listDirectory(argv[i], files);
            }
            else {
                files.push_back(argv[i]);
            }
        }
	}

	loadFiles(files);
	std::cout << files.size() << " files loaded\n";
	std::cout << hm_structures.size() << " structures found\n";

    if (num_errors) {
        for (auto e : error_messages) std::cerr << "ERROR: " << e << "\n";
    }

	return 0;
}
