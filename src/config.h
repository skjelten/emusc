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


#ifndef __CONFIG_H__
#define __CONFIG_H__


#include <map>
#include <string>
#include <vector>


class Config
{
 private:
  std::string _configFilePath;
  std::string _dumpWaveRomPath;
  bool _verbose;
  
  std::map<std::string, std::string> _options;

  void _parse_command_line(int argc, char *argv[]);
  void _show_usage(std::string program);

  std::string _get_config_file_path(void);
  bool _write_default_config(void);
  
  Config();

public:
  Config(int argc, char *argv[]);
  ~Config();


  inline std::string get(std::string key) { return _options[key]; }
  std::vector<int> get_vect_int(std::string key);
  inline bool verbose() { return _verbose; }

};


#endif  // __CONFIG_H__
