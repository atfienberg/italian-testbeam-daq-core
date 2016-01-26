#ifndef DAQ_FAST_CORE_INCLUDE_COMMON_EXTDEF_HH_
#define DAQ_FAST_CORE_INCLUDE_COMMON_EXTDEF_HH_

/*===========================================================================*\

  author: Matthias W. Smith
  email:  mwsmith2@uw.edu
  file:   common_extdef.hh

  about:  Exists to instantiate some of the shared variables for the daq.

  \*===========================================================================*/

#include "common.hh"

namespace daq {

  int vme_dev = -1;
  std::string vme_path("/dev/sis1100_00remote");
  std::mutex vme_mutex;

  // Set the default config directory.
  std::string conf_dir("/home/newg2/Workspace/aaron-work/italian-testbeam-daq/fast/config/");
  // Set up a global zmq context
  zmq::context_t msg_context(1);

  // Set the default logging behavior
  int CommonBase::logging_verbosity_ = 1;
  std::string CommonBase::logfile_("/home/newg2/Workspace/aaron-work/italian-testbeam-daq/fast/fast-daq.log");

  std::fstream CommonBase::logstream_(logfile_);
  std::mutex CommonBase::log_mutex_;

} // ::daq

#endif
