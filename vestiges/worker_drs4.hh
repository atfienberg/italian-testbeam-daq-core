#ifndef DAQ_FAST_CORE_INCLUDE_WORKER_DRS4_HH_
#define DAQ_FAST_CORE_INCLUDE_WORKER_DRS4_HH_

//--- std includes ----------------------------------------------------------//
#include <stdio.h>
#include <stdlib.h>
#include <usb.h>
#include <chrono>
#include <iostream>

//--- other includes --------------------------------------------------------//
#include "vme/sis3100_vme_calls.h"
#include "DRS.h"

//--- project includes ------------------------------------------------------//
#include "worker_base.hh"
#include "common.hh"

// This class pulls data from a drs4 evaluation board.
namespace daq {

class WorkerDrs4 : public WorkerBase<drs4> {

public:
  
  // ctor
  WorkerDrs4(std::string name, std::string conf);

  // dtor
  ~WorkerDrs4() {
    // Clear the data.
    go_time_ = false;
    FlushEvents();
    
    // Stop the thread.
    thread_live_ = false;

    // free the drs4 object.
    delete drs_;
  }
  
  // Read in the configuration from a json file.
  // example file:
  // {
  //     "name":"drs_0",
  //     "device_id":0,
  //     "board_number":0,
  //     "sampling_rate":5.12,
  //     "voltage_range": 0.375,
  //     "trigger_ext":true,
  //     "trigger_chan":1,
  //     "trigger_thresh":0.05,
  //     "trigger_pos":false,
  //     "trigger_delay":150,
  //     "use_drs4_corrections":true
  // }
  void LoadConfig();

  // Collect event data from the device.
  void WorkLoop();

  // Return the oldest event data to the event builder/frontend.
  drs4 PopEvent();
  
private:
  
  std::chrono::high_resolution_clock::time_point t0_;
  bool positive_trg_;

  DRS *drs_;
  DRSBoard *board_;

  // Ask the device it has data.
  bool EventAvailable();

  // Read out the data and add it to the queue.
  void GetEvent(drs4 &bundle);

};

} // ::daq

#endif
