#include "writer_online.hh"

namespace daq {

WriterOnline::WriterOnline(std::string conf_file)
    : WriterBase(conf_file), online_sck_(msg_context, ZMQ_PUSH) {
  thread_live_ = true;
  go_time_ = false;
  end_of_batch_ = false;
  queue_has_data_ = false;
  LoadConfig();

  writer_thread_ = std::thread(&WriterOnline::SendMessageLoop, this);
}

void WriterOnline::LoadConfig() {
  boost::property_tree::ptree conf;
  boost::property_tree::read_json(conf_file_, conf);

  int hwm = conf.get<int>("writers.online.high_water_mark", 10);
  online_sck_.setsockopt(ZMQ_SNDHWM, &hwm, sizeof(hwm));
  int linger = 0;
  online_sck_.setsockopt(ZMQ_LINGER, &linger, sizeof(linger));
  online_sck_.connect(conf.get<std::string>("writers.online.port").c_str());

  max_trace_length_ = conf.get<int>("writers.online.max_trace_length", -1);
}

void WriterOnline::PushData(const std::vector<event_data> &data_buffer) {
  LogMessage("Received some data");

  writer_mutex_.lock();

  number_of_events_ += data_buffer.size();

  auto it = data_buffer.begin();
  while (data_queue_.size() < kMaxQueueSize && it != data_buffer.end()) {
    data_queue_.push(*it);
    ++it;
  }
  queue_has_data_ = true;
  writer_mutex_.unlock();
}

void WriterOnline::EndOfBatch(bool bad_data) {
  FlushData();

  zmq::message_t msg(10);
  memcpy(msg.data(), std::string("__EOB__").c_str(), 10);

  int count = 0;
  while (count < 50) {
    online_sck_.send(msg, ZMQ_DONTWAIT);
    usleep(100);

    count++;
  }
}

void WriterOnline::SendMessageLoop() {
  while (thread_live_) {
    while (go_time_ && queue_has_data_) {
      if (!message_ready_) {
        PackMessage();
      }

      while (message_ready_ && go_time_) {
        int count = 0;
        bool rc = false;
        while (rc == false && count < 200) {
          rc = online_sck_.send(message_, ZMQ_DONTWAIT);
          count++;
        }

        if (rc == true) {
          LogMessage("Sent message successfully");
          message_ready_ = false;
        }

        usleep(daq::short_sleep);
        std::this_thread::yield();
      }

      usleep(daq::short_sleep);
      std::this_thread::yield();
    }

    usleep(daq::long_sleep);
    std::this_thread::yield();
  }
}

void WriterOnline::PackMessage() {
  using boost::uint64_t;

  LogMessage("Packing message.");

  int count = 0;
  char str[50];

  json11::Json::object json_map;

  event_data data;
  {
    std::lock_guard<std::mutex> pack_message_lock(writer_mutex_);

    if (data_queue_.empty()) {
      return;
    }

    data = data_queue_.front();
    data_queue_.pop();
    if (data_queue_.size() == 0) queue_has_data_ = false;

    json_map["event_number"] = number_of_events_;
  }

  for (auto sis : data.sis_3350_vec) {
    json11::Json::object sis_map;
    auto trace_len = max_trace_length_ < 0 ? SIS_3350_LN : max_trace_length_;

    sis_map["system_clock"] = static_cast<double>(sis.system_clock);

    sis_map["device_clock"] =
        std::vector<double>(sis.device_clock, sis.device_clock + SIS_3350_CH);

    std::vector<std::vector<double> > trace_vec;
    for (int ch = 0; ch < SIS_3350_CH; ++ch) {
      trace_vec.emplace_back(sis.trace[ch], sis.trace[ch] + trace_len);
    }
    sis_map["trace"] = trace_vec;

    sprintf(str, "sis_3350_vec_%i", count++);
    json_map[str] = sis_map;
  }

  count = 0;
  for (auto &sis : data.sis_3302_vec) {
    json11::Json::object sis_map;
    auto trace_len = max_trace_length_ < 0 ? SIS_3302_LN : max_trace_length_;

    sis_map["system_clock"] = static_cast<double>(sis.system_clock);

    sis_map["device_clock"] =
        std::vector<double>(sis.device_clock, sis.device_clock + SIS_3302_CH);

    std::vector<std::vector<double> > trace_vec;
    for (int ch = 0; ch < SIS_3302_CH; ++ch) {
      trace_vec.emplace_back(sis.trace[ch], sis.trace[ch] + trace_len);
    }
    sis_map["trace"] = trace_vec;

    sprintf(str, "sis_3302_vec_%i", count++);
    json_map[str] = sis_map;
  }

  count = 0;
  for (auto &sis : data.sis_3316_vec) {
    json11::Json::object sis_map;
    auto trace_len = max_trace_length_ < 0 ? SIS_3316_LN : max_trace_length_;

    sis_map["system_clock"] = static_cast<double>(sis.system_clock);

    sis_map["device_clock"] =
        std::vector<double>(sis.device_clock, sis.device_clock + SIS_3316_CH);

    std::vector<std::vector<double> > trace_vec;
    for (int ch = 0; ch < SIS_3316_CH; ++ch) {
      trace_vec.emplace_back(sis.trace[ch], sis.trace[ch] + trace_len);
    }
    sis_map["trace"] = trace_vec;

    sprintf(str, "sis_3316_vec_%i", count++);
    json_map[str] = sis_map;
  }

  count = 0;
  for (auto &caen : data.caen_6742_vec) {
    json11::Json::object caen_map;
    auto trace_len = max_trace_length_ < 0 ? CAEN_6742_LN : max_trace_length_;

    caen_map["system_clock"] = static_cast<double>(caen.system_clock);

    caen_map["device_clock"] = std::vector<double>(
        caen.device_clock, caen.device_clock + CAEN_6742_CH);

    std::vector<std::vector<double> > trace_vec;
    for (int ch = 0; ch < CAEN_6742_CH; ++ch) {
      trace_vec.emplace_back(caen.trace[ch], caen.trace[ch] + trace_len);
    }
    caen_map["trace"] = trace_vec;

    sprintf(str, "caen_6742_vec_%i", count++);
    json_map[str] = caen_map;
  }

  count = 0;
  for (auto &caen : data.caen_1742_vec) {
    json11::Json::object caen_map;
    auto trace_len = max_trace_length_ < 0 ? CAEN_1742_LN : max_trace_length_;

    caen_map["system_clock"] = static_cast<double>(caen.system_clock);

    caen_map["device_clock"] = std::vector<double>(
        caen.device_clock, caen.device_clock + CAEN_1742_CH);

    std::vector<std::vector<double> > trace_vec;
    for (int ch = 0; ch < CAEN_1742_CH; ++ch) {
      trace_vec.emplace_back(caen.trace[ch], caen.trace[ch] + trace_len);
    }
    caen_map["trace"] = trace_vec;

    std::vector<std::vector<double> > trig_vec;
    for (int gr = 0; gr < CAEN_1742_GR; ++gr) {
      trig_vec.emplace_back(caen.trigger[gr], caen.trigger[gr] + trace_len);
    }
    caen_map["trigger"] = trig_vec;

    sprintf(str, "caen_1742_vec_%i", count++);
    json_map[str] = caen_map;
  }

  count = 0;
  for (auto &board : data.drs4_vec) {
    json11::Json::object drs_map;
    auto trace_len = max_trace_length_ < 0 ? DRS4_LN : max_trace_length_;

    drs_map["system_clock"] = static_cast<double>(board.system_clock);

    drs_map["device_clock"] =
        std::vector<double>(board.device_clock, board.device_clock + DRS4_CH);

    std::vector<std::vector<double> > trace_vec;
    for (int ch = 0; ch < DRS4_CH; ++ch) {
      trace_vec.emplace_back(board.trace[ch], board.trace[ch] + trace_len);
    }
    drs_map["trace"] = trace_vec;

    sprintf(str, "drs4_vec_%i", count++);
    json_map[str] = drs_map;
  }

  count = 0;
  for (auto &caen : data.caen_5720_vec) {
    json11::Json::object caen_map;
    auto trace_len = max_trace_length_ < 0 ? CAEN_5720_LN : max_trace_length_;

    caen_map["system_clock"] = static_cast<double>(caen.system_clock);

    caen_map["event_index"] = static_cast<double>(caen.event_index);

    std::vector<std::vector<double> > trace_vec;
    for (int ch = 0; ch < CAEN_5720_CH; ++ch) {
      trace_vec.emplace_back(caen.trace[ch], caen.trace[ch] + trace_len);
    }
    caen_map["trace"] = trace_vec;

    sprintf(str, "caen5720_vec_%i", count++);
    json_map[str] = caen_map;
  }

  std::string buffer = json11::Json(json_map).dump();
  buffer.append("__EOM__");

  message_ = zmq::message_t(buffer.size());
  std::copy(buffer.begin(), buffer.end(), (char *)message_.data());

  LogMessage("Message ready");
  message_ready_ = true;
}

}  // ::daq
