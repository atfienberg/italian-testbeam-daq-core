#include "sync_client.hh"

namespace daq {

SyncClient::SyncClient() :
  trigger_sck_(msg_context, ZMQ_SUB),
  register_sck_(msg_context, ZMQ_REQ), 
  status_sck_(msg_context, ZMQ_REQ),
  heartbeat_sck_(msg_context, ZMQ_PUB)
{
  DefaultInit();
  InitSockets();
  LaunchThreads();
}

SyncClient::SyncClient(std::string address) :
  trigger_sck_(msg_context, ZMQ_SUB),
  register_sck_(msg_context, ZMQ_REQ), 
  status_sck_(msg_context, ZMQ_REQ),
  heartbeat_sck_(msg_context, ZMQ_PUB)
{
  DefaultInit();

  base_tcpip_ = address;

  InitSockets();
  LaunchThreads();
}

SyncClient::SyncClient(std::string address, int port) :
  trigger_sck_(msg_context, ZMQ_SUB),
  register_sck_(msg_context, ZMQ_REQ), 
  status_sck_(msg_context, ZMQ_REQ),
  heartbeat_sck_(msg_context, ZMQ_PUB)
{
  DefaultInit();

  base_tcpip_ = address;
  base_port_ = port;

  InitSockets();
  LaunchThreads();
}

void SyncClient::DefaultInit()
{
  register_address_ = ConstructAddress(default_tcpip_, default_port_);
  auto uuid = boost::uuids::random_generator()();
  client_name_ = boost::uuids::to_string(uuid) + std::string(";");
  std::cout << "SyncClient: named " << client_name_ << std::endl;

  connected_ = false;
  ready_ = false;
  sent_ready_ = false;
  got_trigger_ = false;
  thread_live_ = true;
}

void SyncClient::InitSockets()
{
  bool rc = false;
  int one = 1;
  int zero = 0;

  zmq::message_t msg(256);
  zmq::message_t reg_msg(client_name_.size());
  std::copy(client_name_.begin(), client_name_.end(), (char *)reg_msg.data());

  // Now the registration socket options.
  register_sck_.setsockopt(ZMQ_RCVTIMEO, &timeout_, sizeof(timeout_));
  register_sck_.setsockopt(ZMQ_SNDTIMEO, &timeout_, sizeof(timeout_));
  register_sck_.setsockopt(ZMQ_IMMEDIATE, &one, sizeof(one));
  register_sck_.setsockopt(ZMQ_LINGER, &zero, sizeof(zero));
  //  register_sck_.setsockopt(ZMQ_REQ_RELAXED, &one, sizeof(one));
  //  register_sck_.setsockopt(ZMQ_REQ_CORRELATE, &one, sizeof(one));

  // Set up the trigger socket options.
  trigger_sck_.setsockopt(ZMQ_RCVTIMEO, &timeout_, sizeof(timeout_));
  trigger_sck_.setsockopt(ZMQ_SNDTIMEO, &timeout_, sizeof(timeout_));
  trigger_sck_.setsockopt(ZMQ_LINGER, &zero, sizeof(zero));

  // Now the status socket
  status_sck_.setsockopt(ZMQ_RCVTIMEO, &timeout_, sizeof(timeout_));
  status_sck_.setsockopt(ZMQ_SNDTIMEO, &timeout_, sizeof(timeout_));
  status_sck_.setsockopt(ZMQ_IMMEDIATE, &one, sizeof(one));
  status_sck_.setsockopt(ZMQ_REQ_RELAXED, &one, sizeof(one));
  status_sck_.setsockopt(ZMQ_REQ_CORRELATE, &one, sizeof(one));
  status_sck_.setsockopt(ZMQ_LINGER, &zero, sizeof(zero));

  // Now finally the heartbeat
  heartbeat_sck_.setsockopt(ZMQ_LINGER, &zero, sizeof(zero));
  

  // Connect the sockets.  The registration socket first.
  register_sck_.connect(register_address_.c_str());

  do {
    rc = register_sck_.send(reg_msg, ZMQ_DONTWAIT);

    if (rc == true) {

      do {
        rc = register_sck_.recv(&msg, ZMQ_DONTWAIT);
        light_sleep();
      } while (!rc && thread_live_);
    }

    heavy_sleep(); // Don't be too aggressive here.

  } while (!rc && thread_live_);

  connected_ = true;

  // Parse the other socket addresses from the message.
  std::string address;
  std::stringstream ss;
  ss.str((char *)msg.data());

  std::getline(ss, address, ';');
  trigger_address_ = address;
  
  std::getline(ss, address, ';');
  status_address_ = address;

  std::getline(ss, address, ';');
  heartbeat_address_ = address;

  std::cout << "trigger address: " << trigger_address_ << std::endl;
  trigger_sck_.connect(trigger_address_.c_str());
  trigger_sck_.setsockopt(ZMQ_SUBSCRIBE, "", 0); // Subscribe post connect

  std::cout << "status address: " << status_address_ << std::endl;
  status_sck_.connect(status_address_.c_str());

  std::cout << "heartbeat address: " << heartbeat_address_ << std::endl;
  heartbeat_sck_.connect(heartbeat_address_.c_str());
}

void SyncClient::LaunchThreads()
{
  status_thread_ = std::thread(&SyncClient::StatusLoop, this);
  restart_thread_ = std::thread(&SyncClient::RestartLoop, this);
  heartbeat_thread_ = std::thread(&SyncClient::HeartbeatLoop, this);
}

void SyncClient::StatusLoop()
{
  std::cout << "StatusLoop launched." << std::endl;

  zmq::message_t msg(256);
  zmq::message_t ready_msg(32);
  zmq::message_t reg_msg(client_name_.size());

  std::string tmp("READY;");

  bool rc = false;
  long last_contact = systime_us();

  while (thread_live_) {

    // Make sure we haven't timed out.
    connected_ = (systime_us() - last_contact) < trigger_timeout_;

    // 1. Make sure we are connected to the trigger.
    if (!connected_) {

      heavy_sleep();

    } else if (!ready_) {

      // Reset the systime_us, so timeouts only occur when looking for triggers.
      last_contact = systime_us();

    } else if (ready_ && !sent_ready_) {
  
      // Request a trigger.
      rc = status_sck_.send(ready_msg, ZMQ_DONTWAIT);

      if (rc == true) {

        // Complete the handshake.
        do {
          rc = status_sck_.recv(&msg, ZMQ_DONTWAIT);
          connected_ = (systime_us() - last_contact) < trigger_timeout_;
        } while (!rc && thread_live_ && connected_);

        if (rc == true) {

          last_contact = systime_us();
          sent_ready_ = true;
        }

      } else {

        light_sleep();
      }

    } else if (ready_ && sent_ready_) {

      // Wait for the trigger
      do { 
        rc = trigger_sck_.recv(&msg, ZMQ_DONTWAIT);
        connected_ = (systime_us() - last_contact) < trigger_timeout_;
      } while (!rc && thread_live_ && connected_);

      // Adjust the flags
      if (rc == true) {

        ready_ = false;
        sent_ready_ = false;

        got_trigger_ = true;
        last_contact = systime_us();
      }
    }

    std::this_thread::yield();
    light_sleep();
  }
}

bool SyncClient::HasTrigger() 
{
  if (got_trigger_ == true) {

    got_trigger_ = false;
    return true;

  } else {

    return false;
  }
}

// Restarts the other sockets and other thread when we lose connection.
void SyncClient::RestartLoop()
{
  std::cout << "RestartLoop launched." << std::endl;

  while(thread_live_) {

    if (!connected_) {
      
      std::cout << "Starting to join threads." << std::endl;
      // Kill the other thread.
      thread_live_ = false;
      if (status_thread_.joinable()) {
        status_thread_.join();
      }
      std::cout << "Joined status_thread." << std::endl;

      if (heartbeat_thread_.joinable()) {
        heartbeat_thread_.join();
      }
      std::cout << "Joined heartbeat_thread." << std::endl;

      zmq_disconnect(trigger_sck_, trigger_address_.c_str());
      zmq_disconnect(register_sck_, register_address_.c_str());
      zmq_disconnect(status_sck_, status_address_.c_str());
      zmq_disconnect(heartbeat_sck_, heartbeat_address_.c_str());

      got_trigger_ = false;
      sent_ready_ = false;
      thread_live_ = true;

      // Reinitialize sockets.
      std::cout << "SyncClient: Reinitializing sockets." << std::endl;
      InitSockets();
      std::cout << "SyncClient: Reinitialization success." << std::endl;
      status_thread_ = std::thread(&SyncClient::StatusLoop, this);
      heartbeat_thread_ = std::thread(&SyncClient::HeartbeatLoop, this);

    } else {

      std::this_thread::yield();
      heavy_sleep();
    }
  }
}

void SyncClient::HeartbeatLoop()
{
  std::cout << "HeartbeatLoop launched." << std::endl;

  // Try to ping every every two long sleep periods.
  while (thread_live_) {

    zmq::message_t msg(client_name_.size());
    std::copy(client_name_.begin(), client_name_.end(),(char *)msg.data());
    heartbeat_sck_.send(msg, ZMQ_DONTWAIT);

    std::this_thread::yield();
    heavy_sleep();
    heavy_sleep();
  }
}

} // ::sp
