/* 
 *  EmuSC - Sound Canvas emulator
 *  Copyright (C) 2022  Håkon Skjelten
 *
 *  EmuSC is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  EmuSC is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with EmuSC.  If not, see <http://www.gnu.org/licenses/>.
 */


#include "config.h"
#include "ex.h"

#include <iostream>
#include <fstream>
#include <sstream>

#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>


Config::Config(int argc, char *argv[])
  : _verbose(false)
{
  bool configFileParam;
  
  _parse_command_line(argc, argv);

  if (_configFilePath.empty()) {
    _configFilePath = _get_config_file_path();
    configFileParam = false;
  } else {
    configFileParam = true;
  }
  
  // Open configuration file read-only
  std::ifstream inFile;
  inFile.open(_configFilePath);
  if (!inFile.is_open()) {
    if (configFileParam) {
      throw(Ex(-1, "Unable to open config file: " + _configFilePath));
    } else {
      if (!_write_default_config())	
	throw(Ex(-1, "Please edit new config file: " + _configFilePath));
      else
	throw(Ex(-1, "Error while writing new config file: " +_configFilePath));
    }
  }

  std::cout << "EmuSC: Configuration file found at " << _configFilePath
	    << std::endl;

  for (std::string line; std::getline(inFile, line);) {
    std::istringstream iss(line);
    std::string id, eq, val;
    iss >> id >> eq >> val >> std::ws;
    if (id[0] != '#' && eq == "=") {
      _options[id] = val;
      if (verbose())
	std::cout << "EmuSC:  -> Config: " << id << " = " << val << std::endl;
    }
  }

  inFile.close();
}

Config::~Config()
{}


void Config::_parse_command_line(int argc, char *argv[])
{
  for (int i = 1; i < argc; i++) {
    std::string param = argv[i];
    if (param == "-h" || param == "--help") {
      _show_usage(std::string(argv[0]));
      throw(Ex(0,""));
    } else if (param == "-d" || param == "--dump-rom-data") {
      if (i < (argc - 1)) {
	_options["dump-rom-data"] = argv[++i];
      }	else {
	_show_usage(std::string(argv[0]));
	throw(Ex(0,"Error: Missing directory to dump decoded PCM data"));
      }
    } else if (param == "-D" || param == "--dump-midi") {
      if (i < (argc - 1)) {
	_options["dump-midi"] = argv[++i];
      }	else {
	_show_usage(std::string(argv[0]));
	throw(Ex(0,"Error: Missing directory to dump MIDI demo songs"));
      }
    } else if (param == "-m" || param == "--mode") {
      if (i < (argc - 1)) {
	_options["mode"] = argv[++i];
      }	else {
	_show_usage(std::string(argv[0]));
	throw(Ex(0,"Error: Missing mode [GS | MT32]"));
      }
    } else if (param == "-u" || param == "--mute") {
      if (i < (argc - 1)) {
	_options["mute"] = argv[++i];
      }	else {
	_show_usage(std::string(argv[0]));
	throw(Ex(0,"Error: Missing comma separated list of parts to mute"));
      }
    } else if (param == "-U" || param == "--mute-except") {
      if (i < (argc - 1)) {
	_options["mute-except"] = argv[++i];
      }	else {
	_show_usage(std::string(argv[0]));
	throw(Ex(0,"Error: Missing comma separated list of parts to NOT mute"));
      }
    } else if (param == "-v" || param == "--verbose") {
      _verbose = true;
    } else if (param == "-c" || param == "--config-file") {
      if (i < (argc - 1)) {
	_configFilePath = argv[++i];
      }	else {
	_show_usage(std::string(argv[0]));
	throw(Ex(0,"Missing path to config file"));
      }
    } else {
      _show_usage(std::string(argv[0]));
      throw(Ex(0,"Unknown parameter: " + std::string(argv[i])));
    }
  }
}


void Config::_show_usage(std::string program)
{
  std::cerr << "Usage: " << program << " [OPTION...]\n\n"
	<< "Options:\n"
	<< "  -d, --dump-pcm-data DIR \tDump decoded PCM data to DIR\n"
        << "  -D, --dump-midi DIR     \tDump MIDI demo songs from ROM to DIR\n"
	<< "  -c, --config PATH       \tUse configuration file at PATH\n"
    	<< "  -m, --mode [GS | MT32]  \tMode of operation. Default: GS\n"
	<< "  -u, --mute PARTS        \tMute parts listed in PARTS [0-15]\n"
    	<< "  -U, --mute-except PARTS \tMute all parts except PARTS [0-15]\n"
	<< "  -h, --help              \tShow this help message\n"
    	<< "  -v, --verbose           \tVerbose terminal output\n"
	<< std::endl;
}


std::string Config::_get_config_file_path(void)
{
  std::string confDirPath;
  struct stat st;

#ifdef WIN32                                    // Windows vista +
  if (!getenv("LOCALAPPDATA"))
    throw(Ex(-1, "%LOCALAPPDATA% variable is required but not defined. Giving up."));

  confDirPath = (std::string) getenv("LOCALAPPDATA") + "\\";
  if (stat(confDirPath.c_str(), &st) == -1)
    if (mkdir(confDirPath.c_str()))
      throw(Ex(-1, "Unable to create config directory at " + confDirPath));

#else                                           // GNU/Linux and all Unixes
  if (!getenv("HOME"))
    throw(Ex(-1, "$HOME variable is required but not defined. Giving up."));

  confDirPath = (std::string) getenv("HOME") + "/.config/emusc/";
  if (stat(confDirPath.c_str(), &st) == -1)
    if (mkdir(confDirPath.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH))
      throw(Ex(-1, "Unable to create config directory at " + confDirPath));
#endif

  return confDirPath + "emusc.conf";
}


bool Config::_write_default_config(void)
{
  std::ofstream configFile;
  configFile.open (_configFilePath);
  if (!configFile.is_open())
    return -1;
  
  configFile << "# Configuration file for EmuSC\n"
	     << std::endl
	     << "# MIDI input system [ alsa | win32 | core | keyboard ]\n"
	     << "input = alsa\n"
	     << std::endl
	     << "# MIDI input device [0 .. ]. Specifies MIDI device id (win32 "
	     << "only)\n"
	     << "#input_device=0\n"
	     << std::endl
	     << "# Audio output system [ alsa | pulse | win32 | core | null ]\n"
	     << "output = alsa\n"
	     << std::endl
	     << "# Output device, e.g. 'default' or 'hw:0.1' for alsa.\n"
	     << "output_device=default\n"
	     << "# Output buffer time in us. Default = 75000\n"
	     << "output_buffer_time=75000\n"
	     << std::endl
	     << "# Output period time in us. Used for alsa. default = 25000\n"
	     << "output_period_time=25000\n"
	     << std::endl
	     << "# ROM files" << std::endl
	     << "control_rom = /SC-55/roland_r15209363.ic23" << std::endl
	     << std::endl
	     << "# Some models use up 3 PCM ROMs (must be in correct order)\n"
	     << "pcm_rom_1 = /SC-55/roland-gss.a_r15209276.ic28" << std::endl
	     << "pcm_rom_2 = /SC-55/roland-gss.b_r15209277.ic27" << std::endl
	     << "pcm_rom_3 = /SC-55/roland-gss.c_r15209281.ic26" << std::endl;
  configFile.close();

  return 0;
}


std::vector<int> Config::get_vect_int(std::string key)
{
  std::vector<int> list;
  std::stringstream ss(_options[key]);

  for (int i; ss >> i;) {
    list.push_back(i);    
    if (ss.peek() == ',')
      ss.ignore();
  }

  return list;
}
