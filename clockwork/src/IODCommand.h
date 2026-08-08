/*
	All rights reserved. Use of this source code is governed by the
	3-clause BSD License in LICENSE.txt.
*/

#ifndef cwlang_Command_h
#define cwlang_Command_h
#include "symboltable.h"
#include "value.h"
#include <stdlib.h>
#include <string.h>
#include <string>
#include <vector>

/*
    class IODString {
    public:
    IODString(char *s) : s_str(0), str(s) { }
    IODString(const char *s) : s_str(s), str(0) { }
    IODString(const IODString &orig) {
        if (orig.str) str = strdup(orig.str);
        s_str = orig.s_str;
    }
    IODString & operator=(const IODString &orig) {
        if (orig.str) str = strdup(orig.str); else str = 0;
        s_str = orig.s_str;
        return *this;
    }
    const char *get() const { return (str) ? str : s_str; }
    ~IODString() { if (str) free( str ); }
    const char *s_str;
    char *str;
    };
*/

class SequenceNumber {
  public:
    int next() { return ++val; }
    int curr() { return val; }
    SequenceNumber() : val(0) {}

  private:
    int val;
};

class IODCommand {
  public:
    IODCommand(int minp = 0, int maxp = 100)
        : done(Unassigned), seqno(sequences.next()), error_str(""), result_str(""),
          min_params(minp), max_params(maxp) {}
    virtual ~IODCommand() {}
    const char *error() { return error_str.c_str(); }
    const char *result() { return result_str.c_str(); }

    enum CommandResult { Failure, Success, Unassigned };
    static SequenceNumber sequences;

    CommandResult done;
    int seqno;

    CommandResult operator()(std::vector<Value> &params) {
        done = (run(params)) ? Success : Failure;
        return done;
    }
    CommandResult operator()() {
        done = (run(parameters)) ? Success : Failure;
        return done;
    }
    void setParameters(std::vector<Value> &);
    const Value &name() {
        if (parameters.empty()) {
            return SymbolTable::Null;
        }
        else {
            return parameters[0];
        }
    }
    size_t numParams() { return parameters.size(); }
    const Value &param(size_t which) {
        if (which < parameters.size()) {
            return parameters[which];
        }
        else {
            return SymbolTable::Null;
        }
    }

    std::ostream &operator<<(std::ostream &) const;

  protected:
    virtual bool run(std::vector<Value> &params) = 0;
    std::vector<Value> parameters;
    std::string error_str;
    std::string result_str;
    int min_params;
    int max_params;
};

std::ostream &operator<<(std::ostream &, const IODCommand &);

#endif
