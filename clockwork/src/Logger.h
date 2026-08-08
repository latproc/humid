/*
	All rights reserved. Use of this source code is governed by the
	3-clause BSD License in LICENSE.txt.
*/

#ifndef cwlang_Logger_h
#define cwlang_Logger_h

#include <libgen.h>
#include <map>
#include <ostream>
#include <set>
#include <sstream>
#include <string>
#include <sys/time.h>
#include <vector>

extern const char *program_name;
class FileLogger {
  public:
    std::ostream &f();
    FileLogger(const char *fname);
    ~FileLogger();

    void getTimeString(char *buf, size_t buf_size);

  private:
    class Internals;
    Internals *internals;
};

class LogState {
  public:
    static LogState *instance();
    static void cleanup();
    int define(const std::string & new_name);
    int lookup(const std::string &name);
    int insert(int flag_num);
    int insert(std::string name);
    void erase(int flag_num);
    void erase(std::string name);
    bool includes(int flag_num);
    bool includes(std::string name);
    std::ostream &operator<<(std::ostream &out) const;

    typedef std::map<std::string, int>::iterator NameMapIterator;

  private:
    LogState();
    LogState(const LogState &) = delete;
    LogState &operator=(const LogState &) = delete;
    static LogState *state_instance;
    std::set<int> state_flags;
    std::vector<std::string> flag_names;
    std::map<std::string, int> name_map;
};

std::ostream &operator<<(std::ostream &, const LogState &);

class Logger {
  public:
    static Logger *instance();

    typedef int Level;
    static int None;
    static int Important;
    static int Debug;
    static int Everything;
    Level level();
    void setLevel(Level n);
    void setLevel(std::string level_name);
    std::ostream &log(Level l);
    void setOutputStream(std::ostream *out);
    static void getTimeString(char *buf, size_t buf_size);
    static void cleanup();

    class Internals;
    static Internals *internals;

  private:
    ~Logger();
    Logger();
};

#define LOGS(l) (LogState::instance()->includes((l)))
#define MSG(l)                                                                                     \
    if (!LogState::instance()->includes((l)))                                                      \
        ;                                                                                          \
    else                                                                                           \
        Logger::instance()->log((l))
#define M_MSG(l, m)                                                                                \
    if (!(m->debug() && LogState::instance()->includes((l))))                                      \
        ;                                                                                          \
    else                                                                                           \
        Logger::instance()->log((l))
#define DBG_MSG MSG(Logger::Debug)
#define DBG_M_MSG M_MSG(Logger::Debug, this)
#define NB_MSG MSG(Logger::Important)
#define INF_MSG MSG(Logger::Everything)

#endif
