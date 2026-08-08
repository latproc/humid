/*
	All rights reserved. Use of this source code is governed by the
	3-clause BSD License in LICENSE.txt.
*/

#ifndef cwlang_State_h
#define cwlang_State_h

#include "value.h"
#include <ostream>
#include <string>

class State {
  public:
    State(const char *name);
    State(int val);
    State(const State &orig);
    virtual ~State();
    State &operator=(const State &other);
    std::ostream &operator<<(std::ostream &out) const;
    virtual bool operator==(const State &other) const;
    virtual bool operator!=(const State &other) const;
    const std::string &getName() const { return text; }
    int getId() const { return token_id; }
    bool is(int tok) { return token_id == tok; }
    Value *getNameValue() { return &name; }
    int getIntValue() { return val; }
    bool isLocal() const { return local; }
    void setLocal(bool l) { local = l; }

    void enter(void *data) const;
    void setEnterFunction(void (*f)(void *));

  private:
    std::string text;
    int val;
    Value name;
    int token_id;
    bool local;
    void (*enter_proc)(void *);
};

std::ostream &operator<<(std::ostream &out, const State &m);

#endif
