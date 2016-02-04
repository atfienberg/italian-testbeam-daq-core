#ifndef DAQ_FAST_CORE_INCLUDE_WORKER_CAEN1742_HH_
#define DAQ_FAST_CORE_INCLUDE_WORKER_CAEN1742_HH_

//--- std includes ----------------------------------------------------------//
#include <chrono>
#include <iostream>
#include <algorithm>

//--- other includes --------------------------------------------------------//

//--- project includes ------------------------------------------------------//
#include "worker_vme.hh"
#include "common.hh"

// This class pulls data from a caen_1742 device.
namespace daq {

typedef struct {
  int16_t cell[CAEN_1742_CH][1024];
  int8_t  nsample[CAEN_1742_CH][1024];
  float   time[CAEN_1742_GR][1024];
} drs_correction;

class WorkerCaen1742 : public WorkerVme<caen_1742> {

 public:
  
  // ctor
  WorkerCaen1742(std::string name, std::string conf);

  // dtor
  ~WorkerCaen1742();

  // Load parameters from a json file.
  // Example file:
  // {
  //     "name":"caen_6742_vec_0",
  //     "base_address": "0xD0000000",
  //     "sampling_rate":1.0,
  //     "pretrigger_delay":50,
  //     "drs_cell_corrections":true,
  //     "drs_peak_corrections":false,
  //     "drs_time_corrections":true,
  //     "channel_offset":[
  // 	     0.15,
  // 	     0.15,
  // 	     0.15,
  // 	     0.15,
  // 	     0.15,
  // 	     0.15,
  // 	     0.15,
  // 	     0.15,
  // 	     0.15,
  // 	     0.15,
  // 	     0.15,
  // 	     0.15,
  // 	     0.15,
  // 	     0.15,
  // 	     0.15,
  // 	     0.15,
  // 	     0.15,
  // 	     0.15,
  // 	     0.15,
  // 	     0.15,
  // 	     0.15,
  // 	     0.15,
  // 	     0.15,
  // 	     0.15,
  // 	     0.15,
  // 	     0.15,
  // 	     0.15,
  // 	     0.15,
  // 	     0.15,
  // 	     0.15,
  // 	     0.15,
  // 	     0.15
  //     ]
  // }
  void LoadConfig();

  // Thread that collects data.
  void WorkLoop();

  // Returns oldest event data to event builder/frontend.
  caen_1742 PopEvent();

private:

  const float vpp_ = 1.0; // Scale of the device's voltage range
  const ushort peakthresh = 30; // For peak corrections
  
  int device_;
  uint sampling_setting_;
  uint size_, bsize_;
  char *buffer_;
  bool drs_cell_corrections_;
  bool drs_peak_corrections_;
  bool drs_time_corrections_;

  std::chrono::high_resolution_clock::time_point t0_;

  //wait for SPI busy flag to be clear
  void WaitForSPI(int group_index);

  // Ask device whether it has data.
  bool EventAvailable();

  // If EventAvailable, read the data and add it to the queue.
  bool GetEvent(caen_1742 &bundle);

  // A function that runs through the three different DRS4 corrections
  // remove effects produce by imperfection in the domino sampling process.
  int ApplyDataCorrection(caen_1742 &data, const std::vector<uint> &startcells);

  // Subtracts off an average inherent bias in the chip based on the sampling
  // start index in the domino ring cycle.
  int CellCorrection(caen_1742 &data, 
		     const drs_correction &table, 
		     const std::vector<uint> &startcells);

  // Check each channel for spikes above threshold and remove it if present
  // in all channels for a group.
  int PeakCorrection(caen_1742 &data, const drs_correction &table);

  // Interpolates values to on evenly spaced grid from the unevenly sampled
  // values reported by the DRS4
  int TimeCorrection(caen_1742 &data, 
		     const drs_correction &table, 
		     const std::vector<uint> &startcells);

  // Readout correction data from the board.
  int GetCorrectionData(drs_correction &table);

  // Readout individual channel's correction data.
  int GetChannelCorrectionData(uint ch, drs_correction &table);
 
  // Read a page of flash memory on the device, used in getting correction data.
  int ReadFlashPage(uint32_t group, uint32_t pagenum, std::vector<int8_t> &page);

  // Optionally write out the correction data as a csv to observe.
  int WriteCorrectionDataCsv();
};

} // ::daq

#endif
