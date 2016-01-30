#include "worker_caenDT5720.hh"

namespace daq {

WorkerCaenDT5720::WorkerCaenDT5720(std::string name, std::string conf)
    : WorkerCaenUSBBase<caen_5720>(name, conf),
      event_(nullptr),
      event_ptr_(nullptr) {
  LoadConfig();
}

WorkerCaenDT5720::~WorkerCaenDT5720() {
  CAEN_DGTZ_SWStopAcquisition(device_);
  CAEN_DGTZ_FreeEvent(device_, (void**)&event_);
}

void WorkerCaenDT5720::LoadConfig() {
  WorkerCaenUSBBase<caen_5720>::LoadConfig();

  CAEN_DGTZ_SetRecordLength(device_, CAEN_5720_LN);
  CAEN_DGTZ_SetChannelEnableMask(device_, 0xf);

  // allocate event
  CAEN_DGTZ_AllocateEvent(device_, (void**)&event_);
  CAEN_DGTZ_SWStartAcquisition(device_);
}

void WorkerCaenDT5720::GetEvent(caen_5720& bundle) {
  LogMessage("getting event!");

  if (CAEN_DGTZ_GetEventInfo(device_, buffer_, bsize_, 0, &event_info_,
                             &event_ptr_)) {
    LogError("failed to get event info");
  }

  LogMessage("event counter %i", event_info_.EventCounter);

  if (CAEN_DGTZ_DecodeEvent(device_, event_ptr_, (void**)&event_)) {
    LogError("could't decode event");
  } else {
    LogMessage("successfully decoded event.");
  }

  bundle.event_index = event_info_.EventCounter;
  auto t1 = std::chrono::high_resolution_clock::now();
  bundle.system_clock =
      std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0_).count();

  for (uint32_t i = 0; i < CAEN_5720_CH; ++i) {
    std::copy(event_->DataChannel[i], event_->DataChannel[i] + CAEN_5720_LN,
              bundle.trace[i]);
  }

  LogMessage("event read out");
}

}  //::daq
