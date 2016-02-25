#include "writer_root.hh"

namespace daq {

  WriterRoot::WriterRoot(std::string conf_file) : WriterBase(conf_file) {
    end_of_batch_ = false;
    LoadConfig();
  }

  void WriterRoot::LoadConfig() {
    boost::property_tree::ptree conf;
    boost::property_tree::read_json(conf_file_, conf);

    outfile_ = conf.get<std::string>("writers.root.file", "default.root");
    tree_name_ = conf.get<std::string>("writers.root.tree", "t");
    need_sync_ = conf.get<bool>("writers.root.sync", false);
  }

  void WriterRoot::StartWriter() {
    using namespace boost::property_tree;

    // Allocate ROOT files
    pf_ = new TFile(outfile_.c_str(), "RECREATE");
    pt_ = new TTree(tree_name_.c_str(), tree_name_.c_str());

    // Turn off autoflush, I will check for synchronization then flush.
    pt_->SetAutoFlush(0);

    // Need to get tree names out of the config file
    ptree conf;
    read_json(conf_file_, conf);

    // For each different device we need to loop and assign branches.
    std::string br_name;
    char br_vars[100];

    // Count the devices, reserve memory for them, then assign an address.
    int count = 0;
    for (auto &v : conf.get_child("devices.sis_3350")) {
      count++;
    }
    for (auto &v : conf.get_child("devices.fake")) {
      count++;
    }
    root_data_.sis_3350_vec.reserve(count + 1);

    count = 0;
    for (auto &v : conf.get_child("devices.sis_3350")) {
      root_data_.sis_3350_vec.resize(count + 1);

      br_name = std::string(v.first);
      sprintf(br_vars, "system_clock/l:device_clock[%i]/l:trace[%i][%i]/s",
	      SIS_3350_CH, SIS_3350_CH, SIS_3350_LN);

      pt_->Branch(br_name.c_str(), &root_data_.sis_3350_vec[count++], br_vars);
    }

    for (auto &v : conf.get_child("devices.fake")) {
      root_data_.sis_3350_vec.resize(count);

      br_name = std::string(v.first);
      sprintf(br_vars, "system_clock/l:device_clock[%i]/l:trace[%i][%i]/s",
	      SIS_3350_CH, SIS_3350_CH, SIS_3350_LN);

      pt_->Branch(br_name.c_str(), &root_data_.sis_3350_vec[count++], br_vars);
    }

    // Again, count, reserve then assign the slow struck.
    count = 0;
    for (auto &v : conf.get_child("devices.sis_3302")) {
      count++;
    }
    root_data_.sis_3302_vec.reserve(count);

    count = 0;
    for (auto &v : conf.get_child("devices.sis_3302")) {
      root_data_.sis_3302_vec.resize(count + 1);

      br_name = std::string(v.first);
      sprintf(br_vars, "system_clock/l:device_clock[%i]/l:trace[%i][%i]/s",
	      SIS_3302_CH, SIS_3302_CH, SIS_3302_LN);

      pt_->Branch(br_name.c_str(), &root_data_.sis_3302_vec[count++], br_vars);
    }

    // Now handle the SIS3316 devices.
    count = 0;
    for (auto &v : conf.get_child("devices.sis_3316")) {
      count++;
    }
    root_data_.sis_3316_vec.reserve(count);

    count = 0;
    for (auto &v : conf.get_child("devices.sis_3316")) {
      root_data_.sis_3302_vec.resize(count + 1);

      br_name = std::string(v.first);
      sprintf(br_vars, "system_clock/l:device_clock[%i]/l:trace[%i][%i]/s",
	      SIS_3316_CH, SIS_3316_CH, SIS_3316_LN);

      pt_->Branch(br_name.c_str(), &root_data_.sis_3302_vec[count++], br_vars);
    }

    // Now set up the caen adc.
    count = 0;
    for (auto &v : conf.get_child("devices.caen_1785")) {
      count++;
    }
    root_data_.caen_1785_vec.reserve(count);

    count = 0;
    for (auto &v : conf.get_child("devices.caen_1785")) {
      root_data_.caen_1785_vec.resize(count + 1);

      br_name = std::string(v.first);
      sprintf(br_vars, "system_clock/l:device_clock[%i]/l:value[%i]/s",
	      CAEN_1785_CH, CAEN_1785_CH);

      pt_->Branch(br_name.c_str(), &root_data_.caen_1785_vec[count++], br_vars);
    }

    // Now set up the caen drs.
    count = 0;
    for (auto &v : conf.get_child("devices.caen_6742")) {
      count++;
    }
    root_data_.caen_6742_vec.reserve(count);

    count = 0;
    for (auto &v : conf.get_child("devices.caen_6742")) {
      root_data_.caen_6742_vec.resize(count + 1);

      br_name = std::string(v.first);
      sprintf(br_vars, "system_clock/l:device_clock[%i]/l:trace[%i][%i]/s",
	      CAEN_6742_CH, CAEN_6742_CH, CAEN_6742_LN);

      pt_->Branch(br_name.c_str(), &root_data_.caen_6742_vec[count++], br_vars);
    }

    // Now set up the drs evaluation board.
    count = 0;
    for (auto &v : conf.get_child("devices.drs4")) {
      count++;
    }
    root_data_.drs4_vec.reserve(count);

    count = 0;
    for (auto &v : conf.get_child("devices.drs4")) {
      root_data_.drs4_vec.resize(count + 1);

      br_name = std::string(v.first);
      sprintf(br_vars, "system_clock/l:device_clock[%i]/l:trace[%i][%i]/s",
	      DRS4_CH, DRS4_CH, DRS4_LN);

      pt_->Branch(br_name.c_str(), &root_data_.drs4_vec[count++], br_vars);
    }

    // Now set up the caen drs vme module.
    count = 0;
    for (auto &v : conf.get_child("devices.caen_1742")) {
      count++;
    }
    root_data_.caen_1742_vec.reserve(count);

    count = 0;
    for (auto &v : conf.get_child("devices.caen_1742")) {
      root_data_.caen_1742_vec.resize(count + 1);

      br_name = std::string(v.first);
      sprintf(
	      br_vars,
	      "system_clock/l:device_clock[%i]/l:trace[%i][%i]/s:trigger[%i][%i]/s",
	      CAEN_1742_CH, CAEN_1742_CH, CAEN_1742_LN, CAEN_1742_GR, CAEN_1742_LN);

      pt_->Branch(br_name.c_str(), &root_data_.caen_1742_vec[count++], br_vars);
    }

    // now the dt5720
    count = 0;
    for (auto &v : conf.get_child("devices.caen_5720")) {
      count++;
    }
    root_data_.caen_5720_vec.reserve(count);

    count = 0;
    for (auto &v : conf.get_child("devices.caen_5720")) {
      root_data_.caen_5720_vec.resize(count + 1);

      br_name = std::string(v.first);
      sprintf(br_vars, "event_index/l:system_clock/l:trace[%i][%i]/s",
	      CAEN_5720_CH, CAEN_5720_LN);

      pt_->Branch(br_name.c_str(), &root_data_.caen_5720_vec[count++], br_vars);
    }

    // now the dt5730
    count = 0;
    for (auto &v : conf.get_child("devices.caen_5730")) {
      count++;
    }
    root_data_.caen_5730_vec.reserve(count);

    count = 0;
    for (auto &v : conf.get_child("devices.caen_5730")) {
      root_data_.caen_5730_vec.resize(count + 1);

      br_name = std::string(v.first);
      sprintf(br_vars, "event_index/l:system_clock/l:trace[%i][%i]/s",
	      CAEN_5730_CH, CAEN_5730_LN);

      pt_->Branch(br_name.c_str(), &root_data_.caen_5730_vec[count++], br_vars);
    }
  }

  void WriterRoot::StopWriter() {
    pf_->Write();
    pf_->Close();

    LogMessage("Closed data TFile.");
    delete pf_;

    std::string cmd("chown venanzoni:venanzoni ");
    cmd += outfile_.c_str();
    system((const char *)cmd.c_str());
  }

  void WriterRoot::PushData(const std::vector<event_data> &data_buffer) {
    for (auto it = data_buffer.begin(); it != data_buffer.end(); ++it) {
      int count = 0;
      for (auto &sis : (*it).sis_3350_vec) {
	root_data_.sis_3350_vec[count++] = sis;
      }

      count = 0;
      for (auto &sis : (*it).sis_3302_vec) {
	root_data_.sis_3302_vec[count++] = sis;
      }

      count = 0;
      for (auto &sis : (*it).sis_3316_vec) {
	root_data_.sis_3316_vec[count++] = sis;
      }

      count = 0;
      for (auto &caen : (*it).caen_1785_vec) {
	root_data_.caen_1785_vec[count++] = caen;
      }

      count = 0;
      for (auto &caen : (*it).caen_6742_vec) {
	root_data_.caen_6742_vec[count++] = caen;
      }

      count = 0;
      for (auto &caen : (*it).caen_1742_vec) {
	root_data_.caen_1742_vec[count++] = caen;
      }

      count = 0;
      for (auto &drs : (*it).drs4_vec) {
	root_data_.drs4_vec[count++] = drs;
      }

      count = 0;
      for (auto &caen : (*it).caen_5720_vec) {
	root_data_.caen_5720_vec[count++] = caen;
      }

      count = 0;
      for (auto &caen : (*it).caen_5730_vec) {
	root_data_.caen_5730_vec[count++] = caen;
      }

      pt_->Fill();

      // Manually flush the baskets.
      //    if (pt_->GetEntries() == 1000) {
      //      pt_->FlushBaskets();
      //    }
    }
  }

  void WriterRoot::EndOfBatch(bool bad_data) {
    LogMessage("Received EOB with bad_data flag = %i", bad_data);

    if (need_sync_ && bad_data) {
      pt_->DropBaskets();
    } else {
      pt_->FlushBaskets();
    }
  }

}  // ::daq
