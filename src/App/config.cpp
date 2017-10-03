//
// Copyright (c) 2016-2017 by Cornell University.  All Rights Reserved.
//
// Permission to use the "TownCrier" software ("TownCrier"), officially
// docketed at the Center for Technology Licensing at Cornell University
// as D-7364, developed through research conducted at Cornell University,
// and its associated copyrights solely for educational, research and
// non-profit purposes without fee is hereby granted, provided that the
// user agrees as follows:
//
// The permission granted herein is solely for the purpose of compiling
// the TownCrier source code. No other rights to use TownCrier and its
// associated copyrights for any other purpose are granted herein,
// whether commercial or non-commercial.
//
// Those desiring to incorporate TownCrier software into commercial
// products or use TownCrier and its associated copyrights for commercial
// purposes must contact the Center for Technology Licensing at Cornell
// University at 395 Pine Tree Road, Suite 310, Ithaca, NY 14850; email:
// ctl-connect@cornell.edu; Tel: 607-254-4698; FAX: 607-254-5454 for a
// commercial license.
//
// IN NO EVENT SHALL CORNELL UNIVERSITY BE LIABLE TO ANY PARTY FOR
// DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES,
// INCLUDING LOST PROFITS, ARISING OUT OF THE USE OF TOWNCRIER AND ITS
// ASSOCIATED COPYRIGHTS, EVEN IF CORNELL UNIVERSITY MAY HAVE BEEN
// ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// THE WORK PROVIDED HEREIN IS ON AN "AS IS" BASIS, AND CORNELL
// UNIVERSITY HAS NO OBLIGATION TO PROVIDE MAINTENANCE, SUPPORT, UPDATES,
// ENHANCEMENTS, OR MODIFICATIONS.  CORNELL UNIVERSITY MAKES NO
// REPRESENTATIONS AND EXTENDS NO WARRANTIES OF ANY KIND, EITHER IMPLIED
// OR EXPRESS, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE, OR THAT THE USE
// OF TOWNCRIER AND ITS ASSOCIATED COPYRIGHTS WILL NOT INFRINGE ANY
// PATENT, TRADEMARK OR OTHER RIGHTS.
//
// TownCrier was developed with funding in part by the National Science
// Foundation (NSF grants CNS-1314857, CNS-1330599, CNS-1453634,
// CNS-1518765, CNS-1514261), a Packard Fellowship, a Sloan Fellowship,
// Google Faculty Research Awards, and a VMWare Research Award.
//

#include "App/config.h"

#include <pwd.h>
#include <sys/types.h>

#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <iostream>

using std::string;
using std::cerr;
using std::cout;
using std::endl;
using std::stringstream;

const tc::Config* g_config;

inline const char *homedir() {
  const char *home_dir;
  if ((home_dir = getenv("HOME")) == NULL) {
    home_dir = getpwuid(getuid())->pw_dir; // NOLINT
  }

  return home_dir;
}

tc::Config::Config(const po::options_description &additional_opts, int argc, const char **argv) {
  this->current_dir = fs::current_path().string();
  this->home_dir = homedir();

  try {
    po::options_description desc("Allowed options");
    desc.add_options()("help,h", "print this message")(
        "rpc", po::bool_switch(&opt_status_enabled)->default_value(DFT_STATUS_ENABLED), "Launch RPC server")(
        "daemon,d", po::bool_switch(&opt_run_as_daemon)->default_value(DFT_RUN_AS_DAEMON), "Run TC as a daemon")(
        "config,c", po::value(&opt_config_file)->default_value(DFT_CONFIG_FILE), "Path to a config file")(
        "cwd", po::value(&opt_working_dir)->default_value(DFT_WORKING_DIR), "Working dir (where log and db are stored");

    desc.add(additional_opts);

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);

    if (vm.count("help")) {
      cerr << desc << endl;
      exit(0);
    }
    po::notify(vm);
  } catch (po::required_option &e) {
    cerr << e.what() << endl;
    exit(-1);
  } catch (std::exception &e) {
    cerr << e.what() << endl;
    exit(-1);
  } catch (...) {
    cerr << "Unknown error!" << endl;
    exit(-1);
  }

  parseConfigFile();
  cout << "config done." << endl;
}

void tc::Config::parseConfigFile() {
  // parse the config files
  boost::property_tree::ptree pt;
  try {
    boost::property_tree::ini_parser::read_ini(opt_config_file, pt);
    cfg_geth_rpc_addr = pt.get<string>("RPC.RPChost");
    cfg_pid_fn = pt.get<string>("daemon.pid_file");
    cfg_status_port = pt.get<int>("status.port");
    cfg_sealed_sig_key = pt.get<string>("sealed.sig_key");
    cfg_sealed_hybrid_key = pt.get<string>("sealed.hybrid_key");
    cfg_enclave_path = pt.get<string>("init.enclave_path");
  } catch (const std::exception &e) {
    std::cout << e.what() << std::endl;
    exit(-1);
  }
}

tc::Config::Config(int argc, const char **argv) {
  this->current_dir = fs::current_path().string();
  this->home_dir = homedir();

  try {
    po::options_description desc("Allowed options");
    desc.add_options()("help,h", "print this message");
    desc.add_options()("measurement", po::bool_switch(&opt_mrenclave)->default_value(false),
                       "print the measurement (MR_ENCLAVE) and exit.");
    desc.add_options()("rpc", po::bool_switch(&opt_status_enabled)->default_value(DFT_STATUS_ENABLED),
                       "Launch RPC server");
    desc.add_options()("daemon,d", po::bool_switch(&opt_run_as_daemon)->default_value(DFT_RUN_AS_DAEMON),
                       "Run TC as a daemon");
    desc.add_options()("config,c", po::value(&opt_config_file)->default_value(DFT_CONFIG_FILE),
                       "Path to a config file");
    desc.add_options()("cwd", po::value(&opt_working_dir)->default_value(DFT_WORKING_DIR),
                       "Working dir (where log and db are stored");

    po::store(po::parse_command_line(argc, argv, desc), vm);

    if (vm.count("help")) {
      cerr << desc << endl;
      exit(0);
    }
    po::notify(vm);
  } catch (po::required_option &e) {
    cerr << e.what() << endl;
    exit(-1);
  } catch (std::exception &e) {
    cerr << e.what() << endl;
    exit(-1);
  } catch (...) {
    cerr << "Unknown error!" << endl;
    exit(-1);
  }

  parseConfigFile();
}

string tc::Config::toString() {
  stringstream ss;
  ss << "status server enabled: " << opt_status_enabled << endl;
  ss << "status server port: " << cfg_status_port << endl;
  ss << "run as daemon: " << opt_run_as_daemon << endl;
  ss << "using config file: " << opt_config_file << endl;
  ss << "working dir set to: " << opt_working_dir << endl;
  ss << "geth rpc addr: " << cfg_geth_rpc_addr << endl;
  ss << "pid filename: " << cfg_pid_fn << endl;
  ss << "enclave image used: " << cfg_enclave_path;

  return ss.str();
}

bool tc::Config::isStatusServerEnabled() const { return opt_status_enabled; }
bool tc::Config::isRunAsDaemon() const { return opt_run_as_daemon; }
const string &tc::Config::getConfigFile() const { return opt_config_file; }
const string &tc::Config::getWorkingDir() const { return opt_working_dir; }
const string &tc::Config::getGethRpcAddr() const { return cfg_geth_rpc_addr; }
int tc::Config::get_status_server_port() const { return cfg_status_port; }
const string &tc::Config::getPidFilename() const { return cfg_pid_fn; }
const string &tc::Config::getSealedSigKey() const { return cfg_sealed_sig_key; }
const string &tc::Config::getSealedHybridKey() const { return cfg_sealed_hybrid_key; }
const string &tc::Config::getEnclavePath() const { return cfg_enclave_path; }
const string &tc::Config::getCurrentDir() const { return current_dir; }
const string &tc::Config::getHomeDir() const { return home_dir; }
bool tc::Config::printMR() const {return opt_mrenclave; }
const po::variables_map& tc::Config::getOpts() const { return vm; }
