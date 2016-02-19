#include "writer_midas.hh"

namespace daq {

WriterMidas::WriterMidas(std::string conf_file) : 
  WriterBase(conf_file), 
  midas_rep_sck_(msg_context, ZMQ_REP), 
  midas_data_sck_(msg_context, ZMQ_PUSH)
{
  thread_live_ = true;
  go_time_ = false;
  end_of_batch_ = false;
  queue_has_data_ = false;
  get_next_event_ = false;
  LoadConfig();

  writer_thread_ = std::thread(&WriterMidas::SendMessageLoop, this);
}

void WriterMidas::LoadConfig()
{
  ptree conf;
  read_json(conf_file_, conf);

  int hwm = conf.get<int>("writers.midas.high_water_mark", 10);
  int linger = 0;
  midas_rep_sck_.setsockopt(ZMQ_LINGER, &linger, sizeof(linger)); 
  midas_rep_sck_.bind(conf.get<std::string>("writers.midas.req_port").c_str());

  midas_data_sck_.setsockopt(ZMQ_SNDHWM, &hwm, sizeof(hwm));
  midas_data_sck_.setsockopt(ZMQ_LINGER, &linger, sizeof(linger)); 
  midas_data_sck_.bind(conf.get<std::string>("writers.midas.data_port").c_str());
}

void WriterMidas::PushData(const std::vector<event_data> &data_buffer)
{
  // Grab only the most recent event

  writer_mutex_.lock();
  while (!data_queue_.empty()) {
    data_queue_.pop();
  }

  if ((data_buffer.size() != 0) && get_next_event_) {
    data_queue_.push(data_buffer[data_buffer.size()-1]);
    queue_has_data_ = true;
  }
  writer_mutex_.unlock();

  LogMessage("Recieved some data.");
}

void WriterMidas::EndOfBatch(bool bad_data)
{
  // while (!data_queue_.empty()) {
  //   SendDataMessage();
  // }

  // zmq::message_t msg(10);
  // memcpy(msg.data(), std::string("__EOB__").c_str(), 10);

  // int count = 0;
  // while (count < 10) {

  //   midas_sck_.send(msg, ZMQ_DONTWAIT);
  //   usleep(100);

  //   count++;
  // }
}

void WriterMidas::SendMessageLoop()
{
  ptree conf;
  read_json(conf_file_, conf);

  bool rc = false;
  zmq::message_t req_msg(20);

  while (thread_live_) {

    while (go_time_) {

      do {
	rc = midas_rep_sck_.recv(&req_msg, ZMQ_NOBLOCK);
      } while ((rc == false) && (zmq_errno() == EAGAIN));
      
      do {
	rc = midas_rep_sck_.send(req_msg, ZMQ_NOBLOCK);
      } while ((rc == false) && (zmq_errno() == EAGAIN));
      
      if (rc == true) {
	get_next_event_ = true;
	usleep(100);
	SendDataMessage();
      }
  
      usleep(daq::short_sleep);
      std::this_thread::yield();
    }
    
    usleep(daq::long_sleep);
    std::this_thread::yield();
  }
}

void WriterMidas::SendDataMessage()
{
  using boost::uint64_t;

  LogMessage("Started sending data");

  int count = 0;
  bool rc = false;
  char str[50];
  std::string data_str;

  // Make sure there is an event
  while(!queue_has_data_) {
    usleep(100);
  }

  LogMessage("Queue got data.");

  // Copy the first event
  writer_mutex_.lock();
  event_data data = data_queue_.front();
  data_queue_.pop();

  if (data_queue_.size() == 0) queue_has_data_ = false;
  get_next_event_ = false;
  writer_mutex_.unlock();

  zmq::message_t som_msg(10);
  std::string msg("__SOM__");
  memcpy(som_msg.data(), msg.c_str(), msg.size()); 

  do {
    midas_data_sck_.send(som_msg, ZMQ_SNDMORE);
  } while ((rc == false) && (zmq_errno() == EINTR));

  // For each device send "<dev_name>:<dev_type>:<binary_data>".
  
  count = 0;
  for (auto &sis : data.sis_3350_vec) {
    
    sprintf(str, "sis_3350_vec_%i:", count++);
    data_str.append(std::string(str));
    data_str.append("sis_3350:");
    
    zmq::message_t data_msg(sizeof(data) + sizeof(sis));
    
    memcpy((char *)data_msg.data(), data_str.c_str(), sizeof(data));
    memcpy((char *)data_msg.data() + sizeof(data), &sis, sizeof(sis));

    do {
      rc = midas_data_sck_.send(data_msg, ZMQ_SNDMORE);
    } while ((rc == false) && (zmq_errno() == EINTR));
  }
  
  count = 0;
  for (auto &sis : data.sis_3302_vec) {

    sprintf(str, "sis_3302_vec_%i:", count++);
    data_str.append(std::string(str));
    data_str.append("sis_3302:");
    
    zmq::message_t data_msg(sizeof(data) + sizeof(sis));
    
    memcpy((char *)data_msg.data(), data_str.c_str(), sizeof(data));
    memcpy((char *)data_msg.data() + sizeof(data), &sis, sizeof(sis));

    do {
      rc = midas_data_sck_.send(data_msg, ZMQ_SNDMORE);
    } while ((rc == false) && (zmq_errno() == EINTR));
  }

  count = 0;
  for (auto &sis : data.sis_3316_vec) {

    sprintf(str, "sis_3316_%i:", count++);
    data_str.append(std::string(str));
    data_str.append("sis_3316:");
    
    zmq::message_t data_msg(sizeof(data) + sizeof(sis));
    
    memcpy((char *)data_msg.data(), data_str.c_str(), sizeof(data));
    memcpy((char *)data_msg.data() + sizeof(data), &sis, sizeof(sis));

    do {
      rc = midas_data_sck_.send(data_msg, ZMQ_SNDMORE);
    } while ((rc == false) && (zmq_errno() == EINTR));
  }

  count = 0;
  for (auto &caen : data.caen_1785_vec) {

    sprintf(str, "caen_1785_vec_%i:", count++);
    data_str.append(std::string(str));
    data_str.append("caen_1785:");
    
    zmq::message_t data_msg(sizeof(data) + sizeof(caen));
    
    memcpy((char *)data_msg.data(), data_str.c_str(), sizeof(data));
    memcpy((char *)data_msg.data() + sizeof(data), &caen, sizeof(caen));

    do {
      rc = midas_data_sck_.send(data_msg, ZMQ_SNDMORE);
    } while ((rc == false) && (zmq_errno() == EINTR));
  }

  count = 0;
  for (auto &caen : data.caen_6742_vec) {

    sprintf(str, "caen_6742_vec_%i:", count++);
    data_str.append(std::string(str));
    data_str.append("caen_6742:");
    
    zmq::message_t data_msg(sizeof(data) + sizeof(caen));
    
    memcpy((char *)data_msg.data(), data_str.c_str(), sizeof(data));
    memcpy((char *)data_msg.data() + sizeof(data), &caen, sizeof(caen));

    do {
      rc = midas_data_sck_.send(data_msg, ZMQ_SNDMORE);
    } while ((rc == false) && (zmq_errno() == EINTR));
  }

  zmq::message_t eom_msg(10);
  msg = std::string("__EOM__:");
  memcpy(eom_msg.data(), msg.c_str(), msg.size()); 
  
  do {
    rc = midas_data_sck_.send(eom_msg);
  } while ((rc == false) && (zmq_errno() == EINTR));

  LogMessage("Finished sending data.");
}

} // ::daq
