#include "worker_caenDT5720.hh"

namespace daq {

WorkerCaenDT5720::WorkerCaenDT5720(std::string name, std::string conf)
    : WorkerCaenUSBBase<caen_5720>(name, conf),
      event_(nullptr),
      event_ptr_(nullptr) {
  LoadConfig();
}

WorkerCaenDT5720::~WorkerCaenDT5720() {
  // make absolutely sure worker thread is done before deallocating
  thread_live_ = go_time_ = false;
  if (work_thread_.joinable()) {
    try {
      work_thread_.join();
    } catch (...) {
      std::cout << name_ << ": thread had race condition joining." << std::endl;
    }
  }

  // free dynamic memory
  if (CAEN_DGTZ_FreeEvent(device_, (void**)&event_)) {
    LogError("Failed to free event");
  }
  if (CAEN_DGTZ_FreeReadoutBuffer(&buffer_)) {
    LogError("Failed to free readout buffer");
  }
}

void WorkerCaenDT5720::LoadConfig() {
  CAEN_DGTZ_SetRecordLength(device_, CAEN_5720_LN);

  CAEN_DGTZ_SetChannelEnableMask(device_, 0xf);

  // disable self trigger
  if (CAEN_DGTZ_SetChannelSelfTrigger(device_, CAEN_DGTZ_TRGMODE_DISABLED,
                                      0xf)) {
    LogError("failed to disable self triggering");
  }

  // set post trigger
  auto trig_delay = conf_.get<uint32_t>("post_trigger_delay");
  if ((trig_delay > 100) || (trig_delay < 0)) {
    LogError(
        "Invalid post trigger value in config. Must be between 0 and 100. "
        "Setting to 50");
    trig_delay = 50;
  }
  if (CAEN_DGTZ_SetPostTriggerSize(device_, trig_delay)) {
    LogError("Error setting post trigger");
  }

  // set channel dc offsets
  int channel_num = 0;
  for (const auto& entry : conf_.get_child("channel_offset")) {
    if (channel_num >= board_info_.Channels) {
      LogError(
          "Too many channels in config dc offsets. This is a %i channel device",
          board_info_.Channels);
      break;
    }

    double val = entry.second.get<double>("");

    if ((val > 1) || (val < 0)) {
      val = 0.5;
      LogError(
          "Invalid channel offset from config. Must be between 0 and 1."
          " Setting to 0.5");
    }

    uint32_t Tvalue = val * 0xffff;
    if (Tvalue > 0xffff) {
      Tvalue = 0xffff;
    } else if (Tvalue < 0) {
      Tvalue = 0;
    }

    if (CAEN_DGTZ_SetChannelDCOffset(device_, channel_num, Tvalue)) {
      LogError("Error setting DC offset to %i for channel %i", Tvalue,
               channel_num);
    }

    ++channel_num;
  }

  // allocate event and buffer
  if (CAEN_DGTZ_MallocReadoutBuffer(device_, &buffer_, &size_)) {
    LogError("failed to allocate readout buffer.");
  }
  if (CAEN_DGTZ_AllocateEvent(device_, (void**)&event_)) {
    LogError("failed to allocate event");
  }
}

caen_5720 WorkerCaenDT5720::GetEvent() {
  LogMessage("getting event!");

  caen_5720 bundle;

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

  return bundle;
}

}  //::daq
