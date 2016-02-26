#ifndef DAQ_FAST_CORE_INCLUDE_WORKER_CAEN6742_HH_
#define DAQ_FAST_CORE_INCLUDE_WORKER_CAEN6742_HH_

//--- std includes ----------------------------------------------------------//
#include <chrono>
#include <iostream>

//--- other includes --------------------------------------------------------//
#include "CAENDigitizer.h"

//--- project includes ------------------------------------------------------//
#include "worker_vme.hh"
#include "common.hh"

// This class pulls data from a caen_6742 device.
namespace daq {

class WorkerCaen6742 : public WorkerBase<caen_6742> {

public:
  
  // ctor
  WorkerCaen6742(std::string name, std::string conf);

  // dtor
  ~WorkerCaen6742();

  // Return the device serial number, used to order USB device consistently.
  int dev_sn() { return board_info_.SerialNumber; };

  // Read in the configuration from a json file.
  // example file:
  // {
  // 	"name":"caen_6742_vec_0",
  // 	"device_id":0,
  //         "sampling_rate":1.0,
  //         "pretrigger_delay":50,
  // 	"use_drs4_corrections":true,
  // 	"channel_offset":[
  // 	    0.15,
  // 	    0.15,
  // 	    0.15,
  // 	    0.15,
  // 	    0.15,
  // 	    0.15,
  // 	    0.15,
  // 	    0.15,
  // 	    0.15,
  // 	    0.15,
  // 	    0.15,
  // 	    0.15,
  // 	    0.15,
  // 	    0.15,
  // 	    0.15,
  // 	    0.15
  // 	]
  // }
  void LoadConfig();

  // Collect event data from the device.
  void WorkLoop();

  // Return the oldest event data to the event builder/frontend.
  caen_6742 PopEvent();

private:

  const float vpp_ = 1.0; // voltage range of device.
  
  int device_;
  uint size_, bsize_;
  char *buffer_;

  std::chrono::high_resolution_clock::time_point t0_;

  CAEN_DGTZ_BoardInfo_t board_info_;
  CAEN_DGTZ_EventInfo_t event_info_;
  CAEN_DGTZ_X742_EVENT_t *event_;
  
  // Ask the device if it has data.
  bool EventAvailable();

  // Read out the data and add it to the queue.
  bool GetEvent(caen_6742 &bundle);

};

} // ::daq

#endif
