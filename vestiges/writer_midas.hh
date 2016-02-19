#ifndef DAQ_FAST_CORE_INCLUDE_WRITER_MIDAS_HH_
#define DAQ_FAST_CORE_INCLUDE_WRITER_MIDAS_HH_

//--- std includes ----------------------------------------------------------//
#include <iostream>
#include <fstream>
#include <queue>

//--- other includes --------------------------------------------------------//
#include <boost/foreach.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
using namespace boost::property_tree;

#include <zmq.hpp>

//--- project includes ------------------------------------------------------//
#include "writer_base.hh"
#include "common.hh"

namespace daq {

// A class that interfaces with the an EventBuilder and writes a root file.

class WriterMidas : public WriterBase {

 public:

  //ctor
  WriterMidas(std::string conf_file);
  
  // Member Functions
  void LoadConfig();
  void StartWriter() { 
    go_time_ = true; 
    number_of_events_ = 0; };
  void StopWriter() { go_time_ = false; };

  // Push data to the writer.
  void PushData(const std::vector<event_data> &data_buffer);

  // Signify that batch is complete and whether it was synchronous.
  void EndOfBatch(bool bad_data);
  
 private:
  
  int number_of_events_;
  std::atomic<bool> go_time_;
  std::atomic<bool> queue_has_data_; 
  std::atomic<bool> get_next_event_;
  std::queue<event_data> data_queue_;
  
  // zmq stuff
  zmq::socket_t midas_rep_sck_;
  zmq::socket_t midas_data_sck_;
  zmq::message_t message_;

  // Pack data into a zmq message.
  void SendDataMessage();

  // Thread used to send data messages to the MIDAS frontend.
  void SendMessageLoop();
};

} // ::daq

#endif
