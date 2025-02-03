/*
 *  This file is part of EmuSC, a Sound Canvas emulator
 *  Copyright (C) 2022-2024  Håkon Skjelten
 *
 *  EmuSC is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published
 *  by the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  EmuSC is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with EmuSC. If not, see <http://www.gnu.org/licenses/>.
 */

// This header file defines all known ROM file sets and their individual SHA256
// values.


#ifndef ROM_INFO_H
#define ROM_INFO_H


#include "emusc/control_rom.h"

#include <vector>
#include <array>
#include <string>


class ROMInfo
{
 public:
  ROMInfo();
  ~ROMInfo();

  struct ControlROMInfo {
    char model[16];
    char version[8];
    char date[12];
    char gsVersion[8];
    std::string progROMsha256;
    std::string cpuROMsha256;
  };

  struct WaveROMInfo {
    char model[16];
    char version[8];
    char date[12];
    int numROMs;
    std::string sha256[3];
  };

  struct ROMSetInfo {
    struct ControlROMInfo controlROMs;
    struct WaveROMInfo waveROMs;
  };

  ROMSetInfo *get_rom_set_info_from_prog(std::string sha256);
  ROMSetInfo *get_rom_set_info_from_cpu(std::string sha256);
  ROMSetInfo *get_rom_set_info_from_wave(std::string sha256, int *index);

 private:
  std::array<struct ROMSetInfo, 2> _romSetList = {{
      { // SC-55 v1.21
        { "SC-55", "1.21", "1991-08-10", "1.13",
          "effc6132d68f7e300aaef915ccdd08aba93606c22d23e580daf9ea6617913af1",
          "7e1bacd1d7c62ed66e465ba05597dcd60dfc13fc23de0287fdbce6cf906c6544", },
        { "SC-55", "0.20", "19xx-xx-xx", 3,
          { "5655509a531804f97ea2d7ef05b8fec20ebf46216b389a84c44169257a4d2007",
            "c655b159792d999b90df9e4fa782cf56411ba1eaa0bb3ac2bdaf09e1391006b1",
            "334b2d16be3c2362210fdbec1c866ad58badeb0f84fd9bf5d0ac599baf077cc2" } }
      }, { // SC-55mkII v1.01 (only known version)
        { "SC-55mkII", "1.01", "1993-07-23", "2.00",
          "a4c9fd821059054c7e7681d61f49ce6f42ed2fe407a7ec1ba0dfdc9722582ce0",
          "8a1eb33c7599b746c0c50283e4349a1bb1773b5c0ec0e9661219bf6c067d2042" },
        { "SC-55mkII", "0.20", "1990-09-12", 2,
          { "c6429e21b9b3a02fbd68ef0b2053668433bee0bccd537a71841bc70b8874243b",
            "5b753f6cef4cfc7fcafe1430fecbb94a739b874e55356246a46abe24097ee491", } }
      }
    }};

};


#endif  // ROM_INFO_H
