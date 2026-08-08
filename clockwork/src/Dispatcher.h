/*
	All rights reserved. Use of this source code is governed by the
	3-clause BSD License in LICENSE.txt.
*/

#pragma once

#include <boost/thread.hpp>
#include <list>
#include <map>
#include <ostream>
#include <string>
#include <utility>
#include "ThreadSafeQueue.h"
#include "Message.h"

class Message;
class Receiver;
struct Package;

class DispatchThread {
  public:
    void operator()();
};

class Dispatcher {
  public:
	using ReceiverList = ThreadSafeList<Receiver*>;
    ReceiverList all_receivers;

    ~Dispatcher();
    std::ostream &operator<<(std::ostream &out) const;
    void deliver(Package *p);
    void deliverZ(Package *p);
    void addReceiver(Receiver *r);
    void removeReceiver(Receiver *r);
    static Dispatcher *create(SharedThreadSafeQueue<Package*> &process_queue);
    static Dispatcher *instance();

    static void start();
    void idle();
    void stop();
    void reset();
    void join();
    void sync_start();

  private:
    Dispatcher(SharedThreadSafeQueue<Package*> &process_queue);
    Dispatcher(const Dispatcher &orig);
    Dispatcher &operator=(const Dispatcher &other);
    bool wait();
    static Dispatcher *instance_;
    bool started;
    bool finished;
    DispatchThread *dispatch_thread;
    boost::thread *thread_ref;
    enum {
        e_waiting,
        e_waiting_cw,
        w_waiting_cmd,
        e_running,
        e_aborted,
        e_handling_dispatch
    } status;
    pthread_t owner_thread;
    SharedThreadSafeQueue<Package*> &process_queue;
    SharedThreadSafeQueue<std::string> command_queue;
    SharedThreadSafeList<Package *> to_deliver;
};

std::ostream &operator<<(std::ostream &out, const Dispatcher &m);
