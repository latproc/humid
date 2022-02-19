/*
	All rights reserved. Use of this source code is governed by the
	3-clause BSD License in LICENSE.txt.
*/

#include "sampletrigger.h"
#include <iostream>

SampleTrigger::SampleTrigger(const std::string prop, const Value val, Kind trigger_type)
    : state(IDLE), property_name(prop), trigger_value(val), kind(trigger_type) {}

SampleTrigger::State SampleTrigger::check(const Value &val) {
    if (ACTIVE) {
        just_triggered = false;
        switch (kind) {
        case EQUAL:
            if (trigger_value == val)
                state = TRIGGERED;
            break;
        case FALLING:
            if (trigger_value <= val && last_check > val)
                state = TRIGGERED;
            break;
        case RISING:
            if (trigger_value >= val && last_check < val)
                state = TRIGGERED;
            break;
        case PASSTHROUGH:
            if ((trigger_value >= val && last_check < val) ||
                (trigger_value <= val && last_check > val))
                state = TRIGGERED;
            break;
        default:;
        }
        if (state == TRIGGERED)
            just_triggered = true;
    }
    return state;
}

void SampleTrigger::setPropertyName(const std::string name) {
    property_name = name;
    reset();
}

#if 0
SampleTrigger::SampleTrigger(const SampleTrigger &orig){
    text = orig.text;
}

SampleTrigger &SampleTrigger::operator=(const SampleTrigger &other) {
    text = other.text;
    return *this;
}

std::ostream &SampleTrigger::operator<<(std::ostream &out) const  {
    out << text;
    return out;
}

std::ostream &operator<<(std::ostream &out, const SampleTrigger &m) {
    return m.operator<<(out);
}

bool SampleTrigger::operator==(const SampleTrigger &other) {
    return text == other.text;
}
#endif
