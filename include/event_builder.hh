#ifndef DAQ_FAST_CORE_INCLUDE_EVENT_BUILDER_HH_
#define DAQ_FAST_CORE_INCLUDE_EVENT_BUILDER_HH_

//--- std includes ----------------------------------------------------------//
#include <ctime>
#include <string>
#include <iostream>
#include <vector>
#include <thread>
#include <atomic>
#include <mutex>
#include <queue>

//--- other includes --------------------------------------------------------//
#include <boost/variant.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

//--- projects includes -----------------------------------------------------//
#include "common.hh"
#include "worker_list.hh"
#include "writer_root.hh"

namespace daq {

// This class pulls data form all the workers.
class EventBuilder : public CommonBase {
 public:
  // ctor
  EventBuilder(const WorkerList &workers,
               const std::vector<WriterBase *> &writers, std::string conf_file);

  // dtor
  ~EventBuilder() {
    std::cout << "Calling EventBuilder destructor." << std::endl;
    thread_live_ = false;
    if (builder_thread_.joinable()) {
      try {
        builder_thread_.join();
      } catch (std::system_error) {
        std::cout << "EventBuilder: thread met race condition." << std::endl;
      }
    }
    if (push_data_thread_.joinable()) {
      try {
        push_data_thread_.join();
      } catch (std::system_error) {
        std::cout << "EventBuilder: thread met race condition." << std::endl;
      }
    }
  }

  // Intended to be called by master frontend at start of run.
  void StartBuilder() { go_time_ = true; };

  // Intended to be called by master frontend at end of run.
  void StopBuilder() { quitting_time_ = true; };

  // Load the configurable parameters from a json file, same as used by master.
  // Example config:
  // {
  //     "trigger_port":"tcp://127.0.0.1:42040",
  //     "handshake_port":"tcp://127.0.0.1:42041",
  //     "batch_size":1,
  //     "max_event_time":1200,
  //     "devices":
  //     {
  //         "fake": {
  //       },
  //         "sis_3350": {
  //         },
  //         "sis_3302": {
  //         },
  //         "caen_1785": {
  //       },
  //         "caen_6742": {
  //       },
  //         "caen_1742": {
  //           "caen_0": "caen_1742_0.json"
  //       },
  //       "drs4": {
  //       }
  //     },
  //     "writers": {
  //        "root": {
  //           "in_use":true,
  //             "file":"data/run_00247.root",
  //             "tree":"t",
  //           "sync":false
  //         },
  //         "online": {
  //           "in_use":true,
  //             "port":"tcp://127.0.0.1:42043",
  //           "high_water_mark":10,
  //           "max_trace_length":1024
  //         },
  //       "midas": {
  //            "in_use":false,
  //             "port":"tcp://127.0.0.1:42044",
  //             "high_water_mark":10
  //       }
  //     },
  //     "trigger_control": {
  //         "live_time":"10000000",
  //         "dead_time":"1"
  //     }
  // }
  void LoadConfig();

  // Allows master frontend to know whether all worker data is packed and sent
  // to the writers.
  bool FinishedRun() { return finished_run_; };

 private:
  // Simple variable declarations
  std::string conf_file_;
  long long batch_start_;
  int max_event_time_;
  int batch_size_;
  const int kMaxQueueSize = 50;

  std::atomic<bool> thread_live_;
  std::atomic<bool> go_time_;
  std::atomic<bool> push_new_data_;
  std::atomic<bool> flush_time_;
  std::atomic<bool> got_last_event_;
  std::atomic<bool> quitting_time_;
  std::atomic<bool> finished_run_;

  // Data accumulation variables
  WorkerList workers_;
  std::vector<WriterBase *> writers_;
  std::vector<event_data> push_data_vec_;
  std::queue<event_data> pull_data_que_;

  // Concurrency variables
  std::mutex queue_mutex_;
  std::mutex push_data_mutex_;
  std::thread builder_thread_;
  std::thread push_data_thread_;

  // Checks for any workers reporting events, then makes sure that
  // no workers have doubles or zeros.
  bool WorkersGotSyncEvent();

  // Copies a batch of data to the queue sending to writers.
  void CopyBatch();

  // Send a full batch to the writers.
  void SendBatch();

  // Send the last batch after receiving quitting_time_ = true.
  void SendLastBatch();

  // Worker control functions.
  void StopWorkers();
  void StartWorkers();

  // Thread the polls workers and packs events.
  void BuilderLoop();

  // Thread that listens for triggers.
  void ControlLoop();
};

}  // ::daq

#endif
