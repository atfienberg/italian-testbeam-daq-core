#include "event_builder.hh"

namespace daq {

EventBuilder::EventBuilder(const WorkerList &workers, 
                           const std::vector<WriterBase *> writers,
                           std::string conf_file) : 
  CommonBase(std::string("EventBuilder"))
{
  workers_ = workers;
  writers_ = writers;
  conf_file_ = conf_file;

  LoadConfig();

  builder_thread_ = std::thread(&EventBuilder::BuilderLoop, this);
  push_data_thread_ = std::thread(&EventBuilder::ControlLoop, this);

}

void EventBuilder::LoadConfig()
{
  boost::property_tree::ptree conf;
  boost::property_tree::read_json(conf_file_, conf);

  thread_live_ = true;
  go_time_ = false;
  quitting_time_ = false;
  finished_run_ = false;

  batch_size_ = conf.get<int>("batch_size", 10);
  max_event_time_ = conf.get<int>("max_event_time", 2000);
}

void EventBuilder::BuilderLoop()
{
  // Thread can only be killed by ending the run.
  while (thread_live_) {

    // Update the reference and drop any events outside of run time.
    batch_start_ = clock();
    workers_.FlushEventData();

    // Collect data while the run isn't paused, in a deadtime or finished.
    while (go_time_) {
      
      if (WorkersGotSyncEvent()) {

	// Get the data.
	event_data bundle;
	
	workers_.GetEventData(bundle);
	
	// Push it back to pull_data queue.
	queue_mutex_.lock();
	if (pull_data_que_.size() < kMaxQueueSize) {
	  pull_data_que_.push(bundle);
	}
	queue_mutex_.unlock();
    
	LogMessage("Data queue is now size = %i", pull_data_que_.size()); 

	//	workers_.FlushEventData();
      }
      
      std::this_thread::yield();
      usleep(daq::short_sleep);

    } // go_time_

    std::this_thread::yield();
    usleep(daq::long_sleep);

  } // thread_live_
}

void EventBuilder::ControlLoop()
{
  while (thread_live_) {

    while (go_time_) {

      if (pull_data_que_.size() >= batch_size_) {
	
        LogMessage("Pushing data");
        CopyBatch();
        SendBatch();
      }

      if (quitting_time_) {
	
	StopWorkers();

	CopyBatch();
	workers_.FlushEventData();

	SendLastBatch();
	
	go_time_ = false;
	thread_live_ = false;
	finished_run_ = true;
      }

      std::this_thread::yield();
      usleep(daq::short_sleep);
    }

    std::this_thread::yield();
    usleep(daq::long_sleep);
  }
}

bool EventBuilder::WorkersGotSyncEvent() 
{
  // Check if anybody has an event.
  if (!workers_.AnyWorkersHaveEvent()) return false;
  LogMessage("Detected a trigger");

  // Wait for all devices to get a chance to read the event.
  usleep(max_event_time_); 

  // Drop the event if not all devices got a trigger.
  if (!workers_.AllWorkersHaveEvent()) {
 
    workers_.FlushEventData();
    LogMessage("Event was not synched");
    return false;
  }
    
  // Drop the event if any devices got two triggers.
  if (workers_.AnyWorkersHaveMultiEvent()) {

    workers_.FlushEventData();
    LogMessage("Trigger was actually a double event");
    return false;
  }

  // Return true if we passed all other checks.
  return true;
}

void EventBuilder::CopyBatch() 
{
  push_data_mutex_.lock();
  push_data_vec_.resize(0);
  
  for (int i = 0; i < batch_size_; ++i) {
    
    queue_mutex_.lock();

    if (!pull_data_que_.empty()) {
      push_data_vec_.push_back(pull_data_que_.front());
      pull_data_que_.pop();

      LogMessage("Pull queue size = %i", pull_data_que_.size());
    }

    queue_mutex_.unlock();
  }

  push_data_mutex_.unlock();
}

void EventBuilder::SendBatch()
{
  push_data_mutex_.lock();

  for (auto &writer : writers_) {
    writer->PushData(push_data_vec_);
  }

  push_data_mutex_.unlock();
}

void EventBuilder::SendLastBatch()
{
  LogMessage("Sending last batch");

  // Zero the pull data vec
  queue_mutex_.lock();
  while (pull_data_que_.size() != 0) pull_data_que_.pop();
  queue_mutex_.unlock();

  push_data_mutex_.lock();
  LogMessage("Sending end of batch/run to the writers");
  for (auto &writer : writers_) {
    
    writer->PushData(push_data_vec_);
    writer->EndOfBatch(false);
  }
  push_data_mutex_.unlock();
}
  

// Start the workers taking data.
void EventBuilder::StartWorkers() {
  LogMessage("Starting workers");
  workers_.StartWorkers();
}

// Write the data file and reset workers.
void EventBuilder::StopWorkers() {
  LogMessage("Stopping workers");
  workers_.StopWorkers();
}

} // ::daq
