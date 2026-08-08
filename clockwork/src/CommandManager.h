/*
	All rights reserved. Use of this source code is governed by the
	3-clause BSD License in LICENSE.txt.
*/

#ifndef cwlang_CommandManager_h
#define cwlang_CommandManager_h

#include "ConnectionManager.h"
#include "MessagingInterface.h"
#include "SocketMonitor.h"
#include <string>
#include <zmq.hpp>

// CommandManager maintain a connection to the command channel of clockwork
class CommandManager : public ConnectionManager {
  public:
    enum Status {
        e_waiting_cmd,
        e_waiting_response,
        e_startup,
        e_disconnected,
        e_waiting_connect,
        e_done
    };
    CommandManager(const char *host, int port);
    void init();
    bool setupConnections();
    bool checkConnections();
    bool checkConnections(zmq::pollitem_t *items, int num_items, zmq::socket_t &cmd);
    virtual int numSocks() { return 1; }
    // Recreate setup REQ after timeout/EFSM (half-open ZMQ_REQ cannot send again).
    void recreateSetupSocket();

    std::string host_name;
    int port;
    zmq::socket_t *setup;
    SingleConnectionMonitor *monit_setup;
    boost::thread *setup_monitor_thread;
    Status setup_status;
    Status run_status;
    // When run_status == e_waiting_response: start time of the outstanding
    // command so a silent remote cannot leave the inproc REP forever mid-reply.
    uint64_t cmd_request_start;
    static const uint64_t cmd_response_timeout_us = 5000000ULL; // 5s
};

#endif
