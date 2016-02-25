#ifndef DAQ_FAST_CORE_INCLUDE_WORKER_DT5730_HH_
#define DAQ_FAST_CORE_INCLUDE_WORKER_DT5730_HH_

#include "worker_caenusb.hh"

namespace daq {

class WorkerCaenDT5730 : public WorkerCaenUSBBase<caen_5730> {
 public:
  WorkerCaenDT5730(std::string name, std::string conf);

  virtual ~WorkerCaenDT5730();

  void LoadConfig();

 protected:
  caen_5730 GetEvent() override;

 private:
  CAEN_DGTZ_UINT16_EVENT_t* event_;
  char* event_ptr_;
};
}  //::daq

#endif
