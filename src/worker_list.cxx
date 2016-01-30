#include "worker_list.hh"
#include "common_extdef.hh"

namespace daq {

void WorkerList::StartRun() {
  StartThreads();
  StartWorkers();
}

void WorkerList::StopRun() {
  StopWorkers();
  StopThreads();
}

struct StartWorkersVis : public boost::static_visitor<> {
  template <typename T>
  void operator()(T workerptr) {
    workerptr->StartWorker();
  }
};

void WorkerList::StartWorkers() {
  // Starts gathering data.
  LogMessage("Starting workers");

  StartWorkersVis vis;
  for (auto &workervar : workers_) {
    boost::apply_visitor(vis, workervar);
  }
}

struct StartThreadsVis : public boost::static_visitor<> {
  template <typename T>
  void operator()(T workerptr) {
    workerptr->StartThread();
  }
};

void WorkerList::StartThreads() {
  // Launches the data worker threads.
  LogMessage("Launching worker threads");

  StartThreadsVis vis;
  for (auto &workervar : workers_) {
    boost::apply_visitor(vis, workervar);
  }
}

struct StopWorkersVis : public boost::static_visitor<> {
  template <typename T>
  void operator()(T workerptr) {
    workerptr->StopWorker();
  }
};

void WorkerList::StopWorkers() {
  // Stop collecting data.
  LogMessage("Stopping workers");

  StopWorkersVis vis;
  for (auto &workervar : workers_) {
    boost::apply_visitor(vis, workervar);
  }
}

struct StopThreadsVis : public boost::static_visitor<> {
  template <typename T>
  void operator()(T workerptr) {
    workerptr->StopThread();
  }
};

void WorkerList::StopThreads() {
  // Stop and rejoin worker threads.
  LogMessage("Stopping worker threads");

  StopThreadsVis vis;
  for (auto &workervar : workers_) {
    boost::apply_visitor(vis, workervar);
  }
}

struct HasEventVis : public boost::static_visitor<bool> {
  template <typename T>
  bool operator()(T workerptr) {
    return workerptr->HasEvent();
  }
};

bool WorkerList::AllWorkersHaveEvent() {
  // Check each worker for an event.
  bool has_event = true;

  HasEventVis vis;
  for (auto &workervar : workers_) {
    has_event &= apply_visitor(vis, workervar);
  }

  return has_event;
}

bool WorkerList::AnyWorkersHaveEvent() {
  // Check each worker for an event.
  bool any_events = false;

  HasEventVis vis;
  for (auto &workervar : workers_) {
    any_events |= apply_visitor(vis, workervar);
  }

  return any_events;
}

struct NumEventsVis : public boost::static_visitor<int> {
  template <typename T>
  int operator()(T workerptr) {
    return workerptr->num_events();
  }
};

bool WorkerList::AnyWorkersHaveMultiEvent() {
  // Check each worker for more than one event.
  NumEventsVis vis;
  for (auto &workervar : workers_) {
    if (apply_visitor(vis, workervar) > 1) {
      return true;
    }
  }

  return false;
}

struct GetEventDataVis : public boost::static_visitor<> {
  void operator()(WorkerBase<sis_3350> *workerptr) {
    bundle->sis_3350_vec.push_back(workerptr->PopEvent());
  }

  void operator()(WorkerBase<sis_3302> *workerptr) {
    bundle->sis_3302_vec.push_back(workerptr->PopEvent());
  }

  void operator()(WorkerBase<caen_1785> *workerptr) {
    bundle->caen_1785_vec.push_back(workerptr->PopEvent());
  }

  void operator()(WorkerBase<caen_6742> *workerptr) {
    bundle->caen_6742_vec.push_back(workerptr->PopEvent());
  }

  void operator()(WorkerBase<drs4> *workerptr) {
    bundle->drs4_vec.push_back(workerptr->PopEvent());
  }

  void operator()(WorkerBase<caen_1742> *workerptr) {
    bundle->caen_1742_vec.push_back(workerptr->PopEvent());
  }

  void operator()(WorkerBase<sis_3316> *workerptr) {
    bundle->sis_3316_vec.push_back(workerptr->PopEvent());
  }

  void operator()(WorkerBase<caen_5720> *workerptr) {
    bundle->caen_5720_vec.push_back(workerptr->PopEvent());
  }

  event_data *bundle;
};

void WorkerList::GetEventData(event_data &bundle) {
  GetEventDataVis vis;
  vis.bundle = &bundle;

  for (auto &workervar : workers_) {
    boost::apply_visitor(vis, workervar);
  }
}

struct FlushEventDataVis : public boost::static_visitor<> {
  template <typename T>
  void operator()(T workerptr) {
    workerptr->FlushEvents();
  }
};

void WorkerList::FlushEventData() {
  // Drops any stale events when workers should have no events.

  FlushEventDataVis vis;
  for (auto &workervar : workers_) {
    boost::apply_visitor(vis, workervar);
  }
}

struct FreeListVis : boost::static_visitor<> {
  template <typename T>
  void operator()(T workerptr) {
    delete workerptr;
  }
};

void WorkerList::FreeList() {
  // Delete the allocated workers.
  LogMessage("Freeing workers");

  FreeListVis vis;
  for (auto &workervar : workers_) {
    boost::apply_visitor(vis, workervar);
  }

  Resize(0);
}

}  // ::daq
