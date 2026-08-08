/*
	All rights reserved. Use of this source code is governed by the
	3-clause BSD License in LICENSE.txt.
*/

#ifndef cwlang_SocketMonitor_h
#define cwlang_SocketMonitor_h

#include "Message.h"
#include "cJSON.h"
#include "symboltable.h"
#include "value.h"
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <zmq.hpp>

#include "MessageEncoding.h"

class EventResponder {
  public:
    virtual ~EventResponder() {}
    virtual void operator()(const zmq_event_t &event_, const char *addr_) = 0;
};

class SocketMonitor : public zmq::monitor_t {
  public:
    SocketMonitor(zmq::socket_t &s);
    virtual ~SocketMonitor();
    void operator()();
    virtual void on_monitor_started();
    virtual void on_event_connected(const zmq_event_t &event_, const char *addr_);
    virtual void on_event_connect_delayed(const zmq_event_t &event_, const char *addr_);
    virtual void on_event_connect_retried(const zmq_event_t &event_, const char *addr_);
    virtual void on_event_listening(const zmq_event_t &event_, const char *addr_);
    virtual void on_event_bind_failed(const zmq_event_t &event_, const char *addr_);
    virtual void on_event_accepted(const zmq_event_t &event_, const char *addr_);
    virtual void on_event_accept_failed(const zmq_event_t &event_, const char *addr_);
    virtual void on_event_closed(const zmq_event_t &event_, const char *addr_);
    virtual void on_event_close_failed(const zmq_event_t &event_, const char *addr_);
    virtual void on_event_disconnected(const zmq_event_t &event_, const char *addr_);
    virtual void on_event_unknown(const zmq_event_t &event_, const char *addr_);
    bool disconnected();
    void abort();
    bool active();
    void addResponder(uint16_t event, EventResponder *responder);
    void removeResponder(uint16_t event, EventResponder *responder);
    // Move event responders to another monitor (used when recreating a socket).
    void transferRespondersFrom(SocketMonitor &other);
    void checkResponders(const zmq_event_t &event_, const char *addr_) const;
    void setMonitorSocketName(std::string name);
    const std::string &monitorSocketName() const;

  protected:
    std::multimap<int, EventResponder *> responders;
    zmq::socket_t &sock;
    bool disconnected_;
    bool aborted;
    bool active_;
    std::string monitor_socket_name;
};

#endif
