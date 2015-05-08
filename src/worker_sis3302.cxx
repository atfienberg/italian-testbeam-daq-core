#include "worker_sis3302.hh"

namespace daq {

WorkerSis3302::WorkerSis3302(std::string name, std::string conf) : 
  WorkerVme<sis_3302>(name, conf)
{
  LoadConfig();

  num_ch_ = SIS_3302_CH;
  read_trace_len_ = SIS_3302_LN / 2; // only for vme ReadTrace
}

void WorkerSis3302::LoadConfig()
{ 
  using std::string;

  int ret;
  uint msg = 0;
  char str[256];

  // Open the configuration file.
  boost::property_tree::ptree conf;
  boost::property_tree::read_json(conf_file_, conf);
  
  string dev_path = conf.get<string>("device");

  // Get the base address for the device.  Convert from hex.
  base_address_ = std::stoi(conf.get<string>("base_address"), nullptr, 0);

  // Read the base register.
  Read(0x0, msg);

  sprintf(str, "%s: found at 0x%08x", name_.c_str(), base_address_);
  WriteLog(str);

  // Reset the device.
  msg = 1;
  if ((ret = Write(0x400, msg)) != 0) {
    std::cerr << name_ << ": error writing sis3302 reset register\n.";
  }

  // Get device ID.
  msg = 0;
  Read(0x4, msg);

  sprintf(str, "%s ID: %04x, maj rev: %02x, min rev: %02x",
          name_.c_str(), msg >> 16, (msg >> 8) & 0xff, msg & 0xff);
  WriteLog(str);

  // Read control/status register.

  if (conf.get<bool>("user_led_on", true))
    msg |= 0x1; // LED on

  msg |= 0xfffe0000; // zero reserved bits

  Write(0x0, msg);

  // Set and check the control/status register.
  msg = 0;

  
  if (conf.get<bool>("invert_ext_lemo")) {
    msg |= 0x10; // invert EXT TRIG
  }

  
  if (conf.get<bool>("user_led_on")) {
    msg |= 0x1; // LED on
  }
  msg = ((~msg & 0xffff) << 16) | msg; // j/k
  msg &= ~0xfffefffe; //reserved bits
  Write(0x0, msg);
  
  msg = 0;
  Read(0x0, msg);

  sprintf(str, "%s user LED: %s", name_.c_str(), (msg & 0x1) ? "ON" : "OFF");
  WriteLog(str);

  // Set Acquisition register.
  msg = 0;
  if (conf.get<bool>("enable_int_stop", true))
    msg |= 0x1 << 6; //enable internal stop trigger

  if (conf.get<bool>("enable_ext_lemo", true))
    msg |= 0x1 << 8; //enable EXT LEMO

  // Set the clock source: 0x0 = Internal, 100MHz
  msg |= conf.get<int>("clock_settings", 0x0) << 12;
  msg = ((~msg & 0xffff) << 16) | msg; // j/k
  msg &= 0x7df07df0; // zero reserved bits / disable bits

  Write(0x10, msg);
  msg = 0;
  Read(0x10, msg);

  sprintf(str, "%s ACQ register set to: 0x%08x", name_.c_str(), msg);
  WriteLog(str);

  // Set the start delay.
  msg = conf.get<int>("start_delay", 0);
  Write(0x14, msg);

  // Set the stop delay.
  msg = conf.get<int>("stop_delay", 0);
  Write(0x18, msg);

  // Read event configure register.
  msg = 0;
  Read(0x02000000, msg);

  // Set event configure register with changes

  if (conf.get<bool>("enable_event_length_stop", true))
    msg = 0x1 << 5; // enable event length stop trigger

  Write(0x01000000, msg);
  
  // Event length register - odd setting method (see manual).
  msg = (SIS_3302_LN - 4 + 512) & 0xfffffc; // @hack - sets number of samples
  Write(0x01000004, msg);

  // Set the pre-trigger buffer length.
  msg = std::stoi(conf.get<string>("pretrigger_samples", "0x0"), nullptr, 0);
  Write(0x01000060, msg);

  // Memory page
  msg = 0; //first 8MB chunk
  Write(0x34, msg);
} // LoadConfig

void WorkerSis3302::WorkLoop()
{
  t0_ = std::chrono::high_resolution_clock::now();

  while (thread_live_) {

    while (go_time_) {

      if (EventAvailable()) {

        static sis_3302 bundle;
        GetEvent(bundle);

        queue_mutex_.lock();
        data_queue_.push(bundle);
        has_event_ = true;
        queue_mutex_.unlock();

      } else {

	std::this_thread::yield();
	usleep(daq::short_sleep);
      }
    }

    std::this_thread::yield();
    usleep(daq::long_sleep);
  }
}

sis_3302 WorkerSis3302::PopEvent()
{
  static sis_3302 data;

  if (data_queue_.empty()) {
    sis_3302 str;
    return str;
  }

  queue_mutex_.lock();

  // Copy the data.
  data = data_queue_.front();
  data_queue_.pop();

  // Check if this is that last event.
  if (data_queue_.size() == 0) has_event_ = false;

  queue_mutex_.unlock();
  return data;
}


bool WorkerSis3302::EventAvailable()
{
  // Check acq reg.
  static uint msg = 0;
  static bool is_event;
  static int count, rc;

  count = 0;
  rc = 0;
  do {
    rc = Read(0x10, msg);
    ++count;
  } while ((rc < 0) && (count < 100));
 
  is_event = !(msg & 0x10000);

  if (is_event && go_time_) {
    // rearm the logic
    uint armit = 1;

    count = 0;
    rc = 0;
    do {
      rc = Write(0x410, armit);
      ++count;
    } while ((rc < 0) && (count < 100));

    return is_event;
  }

  return false;
}

void WorkerSis3302::GetEvent(sis_3302 &bundle)
{
  using namespace std::chrono;
  int ch, offset, ret, count = 0;

  // Check how long the event is.
  //expected SIS_3302_LN + 8
  
  uint next_sample_address[SIS_3302_CH];
  static uint trace[SIS_3302_CH][SIS_3302_LN / 2];
  static uint timestamp[2];

  for (ch = 0; ch < SIS_3302_CH; ch++) {

    next_sample_address[ch] = 0;

    offset = 0x02000010;
    offset |= (ch >> 1) << 24;
    offset |= (ch & 0x1) << 2;

    count = 0;
    ret = 0;
    do {
      ret = Read(offset, next_sample_address[ch]);
      ++count;
    } while ((ret < 0) && (count < 100));
  }

  // Get the system time
  auto t1 = high_resolution_clock::now();
  auto dtn = t1.time_since_epoch() - t0_.time_since_epoch();
  bundle.system_clock = duration_cast<milliseconds>(dtn).count();  

  
  Read(0x10000, timestamp[0]);
  Read(0x10001, timestamp[1]);
  for (ch = 0; ch < SIS_3302_CH; ch++) {
    offset = (0x8 + ch) << 23;

    count = 0;
    ret = 0;
    do {
      ret = ReadTrace(offset, trace[ch]);
      ++count;
    } while ((ret < 0) && (count < 100));
  }

  //decode the event (little endian arch)
  for (ch = 0; ch < SIS_3302_CH; ch++) {

    bundle.device_clock[ch] = 0;
    bundle.device_clock[ch] = timestamp[1] & 0xfff;
    bundle.device_clock[ch] |= (timestamp[1] & 0xfff0000) >> 4;
    bundle.device_clock[ch] |= (timestamp[0] & 0xfffULL) << 24;
    bundle.device_clock[ch] |= (timestamp[0] & 0xfff0000ULL) << 20;

    std::copy((ushort *)trace[ch],
    	      (ushort *)trace[ch] + SIS_3302_LN,
    	      bundle.trace[ch]);
  }
}

} // ::daq
