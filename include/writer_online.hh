#ifndef DAQ_FAST_CORE_INCLUDE_WRITER_ONLINE_HH_
#define DAQ_FAST_CORE_INCLUDE_WRITER_ONLINE_HH_

//--- std includes ----------------------------------------------------------//
#include <iostream>
#include <fstream>
#include <queue>

//--- other includes --------------------------------------------------------//
#include <boost/foreach.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include <zmq.hpp>
#include <json11.hpp>

//--- project includes ------------------------------------------------------//
#include "writer_base.hh"
#include "common.hh"

namespace daq {

// A class that interfaces with the an EventBuilder and writes a root file.

class WriterOnline : public WriterBase {
 public:
  // ctor
  explicit WriterOnline(std::string conf_file);

  // dtor
  ~WriterOnline() {
    go_time_ = false;
    thread_live_ = false;
    if (writer_thread_.joinable()) {
      try {
        writer_thread_.join();
      } catch (const std::system_error& e) {
        LogError("encountered race condition joining thread");
      }
    }
    // Dump data.
    FlushData();
  };

  // Member Functions
  void LoadConfig();
  void StartWriter() {
    go_time_ = true;
    number_of_events_ = 0;
  };
  void StopWriter() { go_time_ = false; };

  void PushData(const std::vector<event_data>& data_buffer);
  void EndOfBatch(bool bad_data);

 private:
  const int kMaxQueueSize = 5;
  int max_trace_length_;
  int number_of_events_;
  std::atomic<bool> message_ready_;
  std::atomic<bool> go_time_;
  std::atomic<bool> queue_has_data_;
  std::queue<event_data> data_queue_;

  // zmq stuff
  zmq::socket_t online_sck_;
  zmq::message_t message_;

  // Pack data into a json stream to pass to the daqometer.
  void PackMessage();

  // Thread that sends data messages to the online monitor.
  void SendMessageLoop();

  // Send all data currently in the queue.
  void FlushData() {
    writer_mutex_.lock();
    while (!data_queue_.empty()) {
      data_queue_.pop();
    }
    queue_has_data_ = false;
    writer_mutex_.unlock();
  };
};

}  // ::daq

#endif
