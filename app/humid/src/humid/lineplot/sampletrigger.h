/*
	All rights reserved. Use of this source code is governed by the
	3-clause BSD License in LICENSE.txt.
*/

#ifndef __SampleTrigger_h__
#define __SampleTrigger_h__

#include <ostream>
#include <string>
#include <lib_clockwork_client.hpp>

/* Sample triggers are polled with the check(value) function.
 	When the check is performed they may change state to 'triggered'
 	The value from the previous poll is retained in order to
 	implement rising/falling semantics (continuous operation is
 	assumed so triggered ->
 		last <= value && rising
 	or	last >= value && falling
 	or	last == value
 */

class SampleTrigger {
public:
    typedef enum { START, STOP } Event;
	typedef enum { EQUAL, RISING, FALLING, ST_PASSTHROUGH } Kind;
	typedef enum { IDLE, ACTIVE, TRIGGERED } State;

	SampleTrigger(const std::string prop,
				  const Value val, Kind trigger_type = EQUAL);
    SampleTrigger(const SampleTrigger &orig);
    SampleTrigger &operator=(const SampleTrigger &other);
    std::ostream &operator<<(std::ostream &out) const;
    bool operator==(const SampleTrigger &other);

	void setType(Kind new_kind) { kind = new_kind; reset(); }
	void setPropertyName(const std::string name);
	void setTriggerValue(Value v) { trigger_value = v; reset(); }

	State check(const Value &val);
	void reset() { state = ACTIVE; last_check = SymbolTable::Null; }
	void cancel() { state = IDLE; }
	bool triggered() const { return state == TRIGGERED; }
	bool active() const { return state == ACTIVE; }

	// when triggered, the justTriggered() method returns true, only
	// until the next check
	bool justTriggered() const { return just_triggered; }


private:
	State state;
	std::string property_name;
	Value trigger_value;
	Kind kind;
	Value last_check;
	bool just_triggered;
};

std::ostream &operator<<(std::ostream &out, const SampleTrigger &m);

#endif
