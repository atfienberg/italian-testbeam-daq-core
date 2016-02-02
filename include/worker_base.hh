#ifndef DAQ_FAST_CORE_INCLUDE_WORKER_BASE_HH_
#define DAQ_FAST_CORE_INCLUDE_WORKER_BASE_HH_

/*===========================================================================*\

  author: Matthias W. Smith
  email:  mwsmith2@uw.edu
  file:   worker_base.hh

  about:  Creates a virtual base class from which new hardware can inherit
          the necessary member functions.  The functions declared here are
          used in the WorkerList class to control many data workers with
          ease.

\*===========================================================================*/

//--- std includes ----------------------------------------------------------//
#include <queue>
#include <atomic>
#include <mutex>
#include <thread>
#include <string>

//--- other includes --------------------------------------------------------//
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

//--- project includes ------------------------------------------------------//
#include "common_base.hh"

namespace daq {

template <typename T>
class WorkerBase : public CommonBase {
 public:
  // Ctor params:
  //   name - used in naming the output data and monitor specific worker
  //   conf_file - used to load important configurable device parameters
  WorkerBase(std::string name, std::string conf_file)
      : thread_live_(true),
        conf_file_(conf_file),
        go_time_(false),
        has_event_(false),
        CommonBase(name) {
    // Change the logfile if there is one in the config.
    boost::property_tree::ptree conf;
    boost::property_tree::read_json(conf_file_, conf);
    SetLogFile(conf.get<std::string>("logfile", logfile_));
  };

  // Dtor rejoins the data pulling thread before destroying the object.
  virtual ~WorkerBase() {
    thread_live_ = false;
    if (work_thread_.joinable()) {
      try {
        work_thread_.join();
      } catch (...) {
        std::cout << name_ << ": thread had race condition joining."
                  << std::endl;
      }
    }
  };

  // Spawns a new thread that pull in new data.
  virtual void StartThread() {
    thread_live_ = true;
    if (work_thread_.joinable()) {
      try {
        work_thread_.join();
      } catch (...) {
        std::cout << name_ << ": thread had race condition joining."
                  << std::endl;
      }
    }
    std::cout << "Launching worker thread. " << std::endl;
    work_thread_ = std::thread(&WorkerBase<T>::WorkLoop, this);
  };

  // Rejoins the data pulling thread.
  virtual void StopThread() {
    thread_live_ = false;
    if (work_thread_.joinable()) {
      try {
        work_thread_.join();
      } catch (const std::system_error& e) {
        std::cout << name_ << ": thread had race condition joining."
                  << std::endl;
      }
    }
  };

  // Exit work loop to idle loop.
  void StartWorker() { go_time_ = true; };

  // Enter work loop from idle loop.
  void StopWorker() { go_time_ = false; };

  // Accessors
  std::string name() { return name_; };
  int num_events() {
    queue_mutex_.lock();
    int size = data_queue_.size();
    queue_mutex_.unlock();
    return size;
  };
  bool HasEvent() { return has_event_; };

  // Pops all stale events on the device.
  void FlushEvents() {
    queue_mutex_.lock();
    while (!data_queue_.empty()) {
      data_queue_.pop();
    }
    queue_mutex_.unlock();
    has_event_ = false;
  };

  // Abstract functions to be implented by descendants.
  virtual void LoadConfig() = 0;
  virtual T PopEvent() = 0;  // T is the classes archetypal data struct

 protected:
  const int max_queue_size_ = 100;
  std::string name_;               // given hardware name
  std::string conf_file_;          // configuration file
  std::atomic<bool> thread_live_;  // keeps paused thread alive
  std::atomic<bool> go_time_;      // controls data taking
  std::atomic<bool> has_event_;    // useful for event building
  std::atomic<int> num_events_;    // useful for synchronization

  std::queue<T> data_queue_;  // stack to hold device events
  std::mutex queue_mutex_;    // mutex to protect data
  std::thread work_thread_;   // thread to launch work loop

  // Constantly checks for an pulls new data onto the data_queue_.
  // Though it can be interrupted by setting go_time_ = false or
  // killed by thread_live_ = false.
  virtual void WorkLoop() = 0;
};

}  // daq

#endif
