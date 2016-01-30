#ifndef DAQ_FAST_CORE_INCLUDE_WORKER_LIST_HH_
#define DAQ_FAST_CORE_INCLUDE_WORKER_LIST_HH_

/*===========================================================================*\

  author: Matthias W. Smith
  email:  mwsmith2@uw.edu
  file:   worker_list.hh

  about:  Creates a vector class that can hold all data workers. This
          class makes event building and run flow much simpler to handle.
          Additionally if new hardware is to be integrated it needs to be
          added to this class.

\*===========================================================================*/

//--- std includes ----------------------------------------------------------//
#include <vector>

//--- other includes --------------------------------------------------------//
#include <boost/variant.hpp>

//--- project includes ------------------------------------------------------//
#include "common.hh"
#include "worker_sis3302.hh"
#include "worker_sis3316.hh"
#include "worker_sis3350.hh"
#include "worker_caen1785.hh"
#include "worker_caen6742.hh"
#include "worker_caen1742.hh"
#include "worker_caenDT5720.hh"
// #include "worker_drs4.hh"

namespace daq {

class WorkerList : public CommonBase {
 public:
  // ctor
  WorkerList() : CommonBase(std::string("WorkerList")){};

  // dtor - the WorkerList takes ownership of workers appended to
  // its worker vector.  They can be freed externally, but we need to
  // be sure.
  ~WorkerList() {
    if (workers_.size() != 0) {
      FreeList();
    }
  };

  // Launches worker threads and starts the collection of data
  void StartRun();

  // Rejoins worker threads and stops the collection of data.
  void StopRun();

  // Launches worker threads.
  void StartThreads();

  // Stops and rejoins worker threads.
  void StopThreads();

  // Starts each worker collecting data.
  void StartWorkers();

  // Stops each worker from collecting data.
  void StopWorkers();

  // Checks if all workers have at least one event.
  bool AllWorkersHaveEvent();

  // Checks if any workers have a single event.
  bool AnyWorkersHaveEvent();

  // Checks if any workers have more than a single event.
  bool AnyWorkersHaveMultiEvent();

  // Copies event data into bundle.
  void GetEventData(event_data &bundle);

  // Flush all stale events.  Each worker has no events after this.
  void FlushEventData();

  // Add a newly allocated worker to the current list.
  void PushBack(worker_ptr_types worker) { workers_.push_back(worker); }

  // Deallocates each worker.
  void FreeList();

  // Return the size of the list.
  int Size() { return workers_.size(); };
  void Resize(int size) { workers_.resize(0); };

 private:
  // This is the actual worker list.
  std::vector<worker_ptr_types> workers_;
};

}  // ::daq

#endif
