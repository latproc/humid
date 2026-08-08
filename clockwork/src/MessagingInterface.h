/*
	All rights reserved. Use of this source code is governed by the
	3-clause BSD License in LICENSE.txt.
*/

#ifndef cwlang_MessagingInterface_h
#define cwlang_MessagingInterface_h

#include "Message.h"
#include "Receiver.h"
#include "cJSON.h"
#include "symboltable.h"
#include "value.h"
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <zmq.hpp>

#include "MessageEncoding.h"
#include "MessageHeader.h"

uint64_t nowMicrosecs(); // deprecated
uint64_t nowMicrosecs(const struct timeval &now);

#if 0
    int64_t get_diff_in_microsecs(const struct timeval *now, const struct timeval *then);
    int64_t get_diff_in_microsecs(uint64_t now, const struct timeval *then);
    int64_t get_diff_in_microsecs(const struct timeval *now, uint64_t then);
#endif

enum ProtocolType { eCLOCKWORK, eRAW, eZMQ, eCHANNEL };

void safeSend(zmq::socket_t &sock, const char *buf, size_t buflen);
void safeSend(zmq::socket_t &sock, const char *buf, size_t buflen, const MessageHeader &header);
void safeSend(zmq::socket_t &sock, const std::string &buf, const MessageHeader &header);

bool safeRecv(zmq::socket_t &sock, char *buf, int buflen, bool block, size_t &response_len,
              int64_t timeout);
bool safeRecv(zmq::socket_t &sock, char **buf, size_t *response_len, bool block, int64_t timeout);
bool safeRecv(zmq::socket_t &sock, char **buf, size_t *response_len, bool block, int64_t timeout,
              MessageHeader &hdr);

bool sendMessage(const char *msg, zmq::socket_t &sock, std::string &response,
                 int32_t timeout_us, const MessageHeader &header);
bool sendMessage(const std::string &msg, zmq::socket_t &sock, std::string &response,
                 int32_t timeout_us = 0);

class MessagingInterface : public Receiver {
  public:
    static const bool DEFERRED_START = true;
    static const bool IMMEDIATE_START = false;

    MessagingInterface(int num_threads, int port, bool deferred_start, ProtocolType proto = eZMQ);
    MessagingInterface(const std::string & host, int port, bool deferred_start, ProtocolType proto = eZMQ);
    ~MessagingInterface();
    void start();
    void stop();
    bool started() const;
    static zmq::context_t *getContext();
    static void setContext(zmq::context_t *);
    static int uniquePort(unsigned int range_start = 7600, unsigned int range_end = 7799);
    char *send(const char *msg);
    char *send(const Message &msg);
    bool send_raw(const char *msg);
    void setCurrent(MessagingInterface *mi) { current = mi; }
    static MessagingInterface *getCurrent();
    static MessagingInterface *create(const std::string &host, int port, ProtocolType proto = eZMQ);
    char *sendCommand(std::string cmd, std::list<Value> *params);
    char *sendCommand(std::string cmd, Value p1 = SymbolTable::Null, Value p2 = SymbolTable::Null,
                      Value p3 = SymbolTable::Null);
    static std::string sendState(const std::string &cmd, const std::string &name, const std::string &state_name);

    //Receiver interface
    virtual bool receives(const Message &, Transmitter *t);
    virtual void handle(const Message &, Transmitter *from, bool needs_receipt);
    virtual void handle(const Message &, Transmitter *from);
    zmq::socket_t *getSocket() { return socket; }
    static bool aborted();
    static void abort();
    const std::string &getURL() const { return url; }

  private:
    static zmq::context_t *zmq_context;
    void connect();
    static MessagingInterface *current;
    ProtocolType protocol;
    zmq::socket_t *socket;
    static std::map<std::string, MessagingInterface *> interfaces;
    bool is_publisher;
    std::string url;
    int connection;
    std::string hostname;
    int port;
    pthread_t owner_thread;
    static bool abort_all;
    bool started_;
};

#endif
