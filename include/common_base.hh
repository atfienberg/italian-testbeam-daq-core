#ifndef DAQ_FAST_CORE_INCLUDE_COMMON_BASE_HH_
#define DAQ_FAST_CORE_INCLUDE_COMMON_BASE_HH_

/*===========================================================================*\

  author: Matthias W. Smith
  email:  mwsmith2@uw.edu
  file:   common_base.hh

  about:  Creates a virtual base class for all daq classes to inherit from.
         Implements a logging scheme.

\*===========================================================================*/

//--- std includes ----------------------------------------------------------//
#include <ctime>
#include <string>
#include <fstream>
#include <iomanip>
#include <cstring>
#include <cstdarg>
#include <mutex>

//--- project includes ------------------------------------------------------//

namespace daq {

class CommonBase {
 public:
  // Ctor params:
  //   name - used in logging output
  explicit CommonBase(std::string name) : name_(name){};
  ~CommonBase(){};

  inline void SetName(std::string name) { name_ = name; };

 protected:
  static const int name_width_ = 15;

  // These should be defined in common_extdef.hh
  static int logging_verbosity_;
  static std::string logfile_;
  static std::fstream logstream_;
  static std::mutex log_mutex_;

  std::string name_;  // given class(hardware) name
  char logstr_[512];

  // Printf style logging function (for max verbosity).
  inline int LogDump(const char* format, ...) {
    if (logging_verbosity_ > 3) {
      log_mutex_.lock();

      while (!logstream_.is_open()) {
        logstream_.open(logfile_, std::fstream::app | std::fstream::out);
        usleep(10);
      }

      prepend(level::DUMP);

      va_list args;
      va_start(args, format);
      vsprintf(logstr_, format, args);
      va_end(args);

      logstream_ << logstr_ << std::endl;
      logstream_.close();
      log_mutex_.unlock();
    }

    return 0;
  };

  // Prints a simple string to the log file (for max verbosity).
  inline int LogDump(const std::string& message) {
    if (logging_verbosity_ > 3) {
      log_mutex_.lock();

      while (!logstream_.is_open()) {
        logstream_.open(logfile_, std::fstream::app | std::fstream::out);
        usleep(10);
      }

      prepend(level::DUMP);

      logstream_ << message << std::endl;

      logstream_.close();
      log_mutex_.unlock();
    }

    return 0;
  };

  // Printf style logging function (for debug verbosity).
  inline int LogDebug(const char* format, ...) {
    if (logging_verbosity_ > 2) {
      log_mutex_.lock();

      while (!logstream_.is_open()) {
        logstream_.open(logfile_, std::fstream::app | std::fstream::out);
        usleep(10);
      }

      prepend(level::DEBUG);

      va_list args;
      va_start(args, format);
      vsprintf(logstr_, format, args);
      va_end(args);

      logstream_ << logstr_ << std::endl;
      logstream_.close();
      log_mutex_.unlock();
    }

    return 0;
  };

  // Prints a simple string to the log file (for debug verbosity).
  inline int LogDebug(const std::string& message) {
    if (logging_verbosity_ > 2) {
      log_mutex_.lock();

      while (!logstream_.is_open()) {
        logstream_.open(logfile_, std::fstream::app | std::fstream::out);
        usleep(10);
      }

      prepend(level::DEBUG);

      logstream_ << message << std::endl;

      logstream_.close();
      log_mutex_.unlock();
    }

    return 0;
  };

  // Printf style logging function (verbosity = 2).
  inline int LogMessage(const char* format, ...) {
    if (logging_verbosity_ > 1) {
      log_mutex_.lock();

      while (!logstream_.is_open()) {
        logstream_.open(logfile_, std::fstream::app | std::fstream::out);
        usleep(10);
      }

      prepend(level::MESSAGE);

      va_list args;
      va_start(args, format);
      vsprintf(logstr_, format, args);
      va_end(args);

      logstream_ << logstr_ << std::endl;
      logstream_.close();
      log_mutex_.unlock();
    }

    return 0;
  };

  // Prints string to log file (verbosity = 2).
  inline int LogMessage(const std::string& message) {
    if (logging_verbosity_ > 1) {
      log_mutex_.lock();

      while (!logstream_.is_open()) {
        logstream_.open(logfile_, std::fstream::app | std::fstream::out);
        usleep(10);
      }

      prepend(level::MESSAGE);

      logstream_ << message << std::endl;

      logstream_.close();
      log_mutex_.unlock();
    }

    return 0;
  };

  // Printf style logging function (verbosity = 1).
  inline int LogWarning(const char* format, ...) {
    if (logging_verbosity_ > 0) {
      log_mutex_.lock();

      while (!logstream_.is_open()) {
        logstream_.open(logfile_, std::fstream::app | std::fstream::out);
        usleep(10);
      }

      prepend(level::WARNING);

      va_list args;
      va_start(args, format);
      vsprintf(logstr_, format, args);
      va_end(args);

      logstream_ << logstr_ << std::endl;

      logstream_.close();
      log_mutex_.unlock();
    }

    return 0;
  };

  // Prints string to log file (verbosity = 1).
  inline int LogWarning(const std::string& warning) {
    if (logging_verbosity_ > 0) {
      log_mutex_.lock();

      while (!logstream_.is_open()) {
        logstream_.open(logfile_, std::fstream::app | std::fstream::out);
        usleep(10);
      }

      prepend(level::WARNING);

      logstream_ << warning << std::endl;

      logstream_.close();
      log_mutex_.unlock();
    }

    return 0;
  };

  // Printf style logging function (always).
  inline int LogError(const char* format, ...) {
    log_mutex_.lock();

    while (!logstream_.is_open()) {
      logstream_.open(logfile_, std::fstream::app | std::fstream::out);
      usleep(10);
    }

    prepend(level::ERROR);

    va_list args;
    va_start(args, format);
    vsprintf(logstr_, format, args);
    va_end(args);

    logstream_ << logstr_ << std::endl;

    logstream_.close();
    log_mutex_.unlock();

    return 0;
  };

  // Prints string to log file (always).
  inline int LogError(const std::string& error) {
    log_mutex_.lock();

    while (!logstream_.is_open()) {
      logstream_.open(logfile_, std::fstream::app | std::fstream::out);
      usleep(10);
    }

    prepend(level::ERROR);

    logstream_ << error << std::endl;

    logstream_.close();
    log_mutex_.unlock();

    return 0;
  };

  // User to override the default log file (/var/log/lab-daq/fast-daq.log).
  void SetLogFile(const std::string& logfile) {
    log_mutex_.lock();
    logfile_ = logfile;
    log_mutex_.unlock();
  };

 private:
  enum level { DUMP, DEBUG, MESSAGE, WARNING, ERROR };

  inline void prepend(level lvl) {
    static timespec t;
    static char tm_start[100];
    static std::string lvl_msg;

    clock_gettime(CLOCK_REALTIME, &t);
    std::strftime(tm_start, sizeof(tm_start), "[%F %T.", localtime(&t.tv_sec));

    switch (lvl) {
      case DUMP:
        lvl_msg = std::string("]  ==DUMP== ");
        break;
      case DEBUG:
        lvl_msg = std::string("]  =DEBUG=  ");
        break;
      case MESSAGE:
        lvl_msg = std::string("]  MESSAGE  ");
        break;
      case WARNING:
        lvl_msg = std::string("] *WARNING* ");
        break;
      case ERROR:
        lvl_msg = std::string("] **ERROR** ");
        break;
    }

    logstream_ << tm_start << std::right << std::setfill('0') << std::setw(6);
    logstream_ << t.tv_nsec / 1000 << lvl_msg << std::setfill(' ') << "{ ";
    logstream_ << std::left << std::setw(name_width_) << name_ << " } : ";
  }
};

}  // daq

#endif
