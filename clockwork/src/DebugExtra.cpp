/*
	All rights reserved. Use of this source code is governed by the
	3-clause BSD License in LICENSE.txt.
*/

#include "DebugExtra.h"
#include "Logger.h"

DebugExtra *DebugExtra::instance_ = 0;
DebugExtra::DebugExtra() {
    auto logstate = LogState::instance();
    DEBUG_PARSER = logstate->define("DEBUG_PARSER");
    DEBUG_PREDICATES = logstate->define("DEBUG_PREDICATES");
    DEBUG_MESSAGING = logstate->define("DEBUG_MESSAGING");
    DEBUG_STATECHANGES = logstate->define("DEBUG_STATECHANGES");
    DEBUG_SCHEDULER = logstate->define("DEBUG_SCHEDULER");
    DEBUG_AUTOSTATES = logstate->define("DEBUG_AUTOSTATES");
    DEBUG_MACHINELOOKUPS = logstate->define("DEBUG_MACHINELOOKUPS");
    DEBUG_PROPERTIES = logstate->define("DEBUG_PROPERTIES");
    DEBUG_DEPENDANCIES = logstate->define("DEBUG_DEPENDANCIES");
    DEBUG_ACTIONS = logstate->define("DEBUG_ACTIONS");
    DEBUG_INITIALISATION = logstate->define("DEBUG_INITIALISATION");
    DEBUG_MODBUS = logstate->define("DEBUG_MODBUS");
    DEBUG_DISPATCHER = logstate->define("DEBUG_DISPATCHER");
    DEBUG_CHANNELS = logstate->define("DEBUG_CHANNELS");
    DEBUG_ETHERCAT = logstate->define("DEBUG_ETHERCAT");
    DEBUG_PROCESSING = logstate->define("DEBUG_PROCESSING");
    DEBUG_ETHERCAT_CALLS = logstate->define("DEBUG_ETHERCAT_CALLS");
    DEBUG_ETHERCAT_SDO = logstate->define("DEBUG_ETHERCAT_SDO");
    DEBUG_ETHERCAT_PACKETS = logstate->define("DEBUG_ETHERCAT_PACKETS");
    DEBUG_PROCSNAP = logstate->define("DEBUG_PROCSNAP");
    DEBUG_MEMSNAPSHOT = logstate->define("DEBUG_MEMSNAPSHOT");
}

DebugExtra *DebugExtra::instance() {
    if (!instance_) {
        instance_ = new DebugExtra;
    }
    return instance_;
}
