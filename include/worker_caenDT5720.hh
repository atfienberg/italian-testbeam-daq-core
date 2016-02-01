#ifndef DAQ_FAST_CORE_INCLUDE_WORKER_DT5720_HH_
#define DAQ_FAST_CORE_INCLUDE_WORKER_DT5720_HH_

#include "worker_caenusb.hh"

namespace daq {

class WorkerCaenDT5720 : public WorkerCaenUSBBase<caen_5720> {
 public:
  WorkerCaenDT5720(std::string name, std::string conf);

  virtual ~WorkerCaenDT5720();

  void LoadConfig();

 protected:
  caen_5720 GetEvent() override;

 private:
  CAEN_DGTZ_UINT16_EVENT_t* event_;
  char* event_ptr_;
};
}  //::daq

#endif
