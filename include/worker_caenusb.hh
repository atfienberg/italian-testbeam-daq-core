#ifndef DAQ_FAST_CORE_INCLUDE_WORKER_CAENUSBBASE_HH_
#define DAQ_FAST_CORE_INCLUDE_WORKER_CAENUSBBASE_HH_

//--- std includes ----------------------------------------------------------//
#include <chrono>

//--- other includes --------------------------------------------------------//
#include "CAENDigitizer.h"

//--- project includes ------------------------------------------------------//
#include "worker_base.hh"
#include "common.hh"

namespace daq {

/**
 * base class for caen usb digitizers
 */
template <typename T>
class WorkerCaenUSBBase : public WorkerBase<T> {
 public:
  WorkerCaenUSBBase(std::string name, std::string conf)
      : WorkerBase<T>(name, conf),
	device_(0),
        size_(0),
        bsize_(0),
        buffer_(nullptr) {
    LoadConfig();
  }

  virtual ~WorkerCaenUSBBase() {
    CAEN_DGTZ_ErrorCode ret;
    if(ret = CAEN_DGTZ_CloseDigitizer(device_)){
      this->LogError("failed to close digitizer, error code %i", ret);
    }
  }

  void LoadConfig() override;

  T PopEvent() override;

 protected:
  virtual T GetEvent() = 0;

  virtual void StartAcquisition() {  
    if(CAEN_DGTZ_SWStartAcquisition(device_)){
      this->LogError("failed to start acquisition.");
    }
  }

  virtual void StopAcquisition() {
    if(CAEN_DGTZ_SWStopAcquisition(device_)){
      this->LogError("failed to stop acquisition.");
    }
  }

  void WorkLoop() override;

  boost::property_tree::ptree conf_;

  std::chrono::high_resolution_clock::time_point t0_;

  int device_;

  uint32_t size_, bsize_;
  char* buffer_;

  CAEN_DGTZ_BoardInfo_t board_info_;
  CAEN_DGTZ_EventInfo_t event_info_;

 private:
  bool EventAvailable();
};

template <typename T>
void WorkerCaenUSBBase<T>::LoadConfig() {
  boost::property_tree::read_json(this->conf_file_, conf_);

  CAEN_DGTZ_ErrorCode ret;

  int id = conf_.get<int>("device_id");

  if (ret = CAEN_DGTZ_OpenDigitizer(CAEN_DGTZ_USB, id, 0, 0, &device_)) {
    this->LogError("failed to open device, error code %i", ret);
  }

  if (CAEN_DGTZ_Reset(device_)) {
    this->LogError("failed to reset device");
  }

  if (CAEN_DGTZ_GetInfo(device_, &board_info_)) {
    this->LogError("failed to get board info");
  } else {
    this->LogMessage("Found caen %s.", board_info_.ModelName);
    this->LogMessage("Serial Number: %i.", board_info_.SerialNumber);
  }
  
  this->LogMessage("set sw trigger mode");
  if (CAEN_DGTZ_SetSWTriggerMode(device_, CAEN_DGTZ_TRGMODE_ACQ_ONLY)) {
    this->LogError("failed to enable sw triggers");
  }

  if (CAEN_DGTZ_SetExtTriggerInputMode(device_,
                                       CAEN_DGTZ_TRGMODE_ACQ_AND_EXTOUT)) {
    this->LogMessage("failed to enable ext trigger");
  }

  if (CAEN_DGTZ_SetMaxNumEventsBLT(device_, 1)) {
    this->LogMessage("failed to set max BLT events");
  }

  // rest of stuff should be done in base class
  // set acquisition mode, allocate buffers, 
  // device specific settings, etc
}

template <typename T>
void WorkerCaenUSBBase<T>::WorkLoop() {
  CAEN_DGTZ_ErrorCode ret;

  t0_ = std::chrono::high_resolution_clock::now();
  
  StartAcquisition();
  while (this->thread_live_) {
    while (this->go_time_) {
      if (EventAvailable()) {
        T bundle = GetEvent();

        std::lock_guard<std::mutex> lock(this->queue_mutex_);
        this->data_queue_.push(bundle);
        this->has_event_ = true;
      } else {
        std::this_thread::yield();
        usleep(daq::short_sleep);
      }
    }

    std::this_thread::yield();
    usleep(daq::long_sleep);
  }
  StopAcquisition();
}

template <typename T>
T WorkerCaenUSBBase<T>::PopEvent() {
  std::lock_guard<std::mutex> lock(this->queue_mutex_);

  if (this->data_queue_.empty()) {
    T empty_structure;
    return empty_structure;

  } else {
    // Copy the data.
    T data = this->data_queue_.front();
    this->data_queue_.pop();

    // Check if this is that last event.
    if (this->data_queue_.size() == 0) this->has_event_ = false;

    return data;
  }
}

template <typename T>
bool WorkerCaenUSBBase<T>::EventAvailable() {
  if (CAEN_DGTZ_ReadData(device_, CAEN_DGTZ_SLAVE_TERMINATED_READOUT_MBLT,
                         buffer_, &bsize_)) {
    this->LogError("failed to read data");
  }

  uint32_t num_events;
  if (CAEN_DGTZ_GetNumEvents(device_, buffer_, bsize_, &num_events)) {
    this->LogError("failed to get num events");
  }

  return num_events > 0;
}

}  // ::daq
#endif
