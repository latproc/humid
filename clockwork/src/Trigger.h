/*
	All rights reserved. Use of this source code is governed by the
	3-clause BSD License in LICENSE.txt.
*/

#pragma once

#include "Expression.h"
#include "Message.h"
#include <boost/foreach.hpp>
#include <cstdlib>
#include <cstring>
#include <set>
#include <sys/types.h>
#include <vector>

typedef std::vector<std::string> ActionParameterList;
class MachineInstance;

class Action;
class TriggerInternals;
class Trigger;

struct TriggerOwner {
    virtual ~TriggerOwner() = default;
    virtual void triggerFired(Trigger *trigger) {};
};

class Trigger {
  public:
    explicit Trigger(const std::string &n);
    Trigger(TriggerOwner *, const std::string &n);

    virtual ~Trigger();
    Trigger *retain();
    virtual Trigger *release();
    static char *getTriggers();
    static size_t liveCount();
    void addHolder(Action *h);
    void removeHolder(Action *h);

    void setOwner(TriggerOwner *new_owner);
    bool enabled() const;
    bool fired() const;
    void fire();
    void disable();
    virtual const std::string &getName() const;
    //const std::string &getName();
    bool matches(const std::string &event);

    uint64_t startTime();
    void report(const char *message);
    int getRefs() const { return refs; }

    std::ostream &operator<<(std::ostream &out) const;

  protected:
    TriggerInternals *_internals;
    std::string name;
    bool seen;
    TriggerOwner *owner;
    bool deleted;
    int refs;

  private:
    bool is_active;

  public:
    // None of these constructors and assignment operators are
    // implemented because they are not supported
    Trigger() = delete;
    Trigger(const Trigger &o) = delete;
    Trigger &operator=(const Trigger &o) = delete;
};
std::ostream &operator<<(std::ostream &out, const Trigger &t);
