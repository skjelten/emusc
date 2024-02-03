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

// This header file defines all known ROM files, their capabilites and SHA256.


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

  struct CtrlROMInfo {
    EmuSC::ControlRom::SynthGen SynthGen;
    char model[16];
    char version[8];
    char date[12];
    char gsVersion[8];
    std::string sha256;
  };

  struct PcmROMInfo {
    EmuSC::ControlRom::SynthGen SynthGen;
    char model[16];
    char version[8];
    char date[12];
    std::string sha256[4];
  };

  CtrlROMInfo *get_control_rom_info(std::string sha256);
  PcmROMInfo *get_pcm_rom_info(std::string sha256);

  static int get_number_of_pcm_rom_files(PcmROMInfo *romInfo);
  static int get_pcm_rom_index(PcmROMInfo *romInfo, std::string sha256);

 private:
  // Definition of all supported ROM files
  std::array<struct CtrlROMInfo, 11> _ctrlROMList = {{
    { EmuSC::ControlRom::SynthGen::SC55, "SC-55",
      "1.20", "1991-04-06", "1.13",
      "22ce6ca59e6332143b335525e81fab501ea6fccce4b7e2f3bfc2cc8bf6612ff6" },
    { EmuSC::ControlRom::SynthGen::SC55, "SC-55",
      "1.21", "1991-08-10", "1.13",
      "effc6132d68f7e300aaef915ccdd08aba93606c22d23e580daf9ea6617913af1" },
    { EmuSC::ControlRom::SynthGen::SC55mk2, "SC-55mkII",
      "1.01", "1993-07-23", "2.00",
      "a4c9fd821059054c7e7681d61f49ce6f42ed2fe407a7ec1ba0dfdc9722582ce0" },
    { EmuSC::ControlRom::SynthGen::SC55, "SCC-1",
      "?.??", "19xx-xx-xx", "1.10",
      "0283d32e6993a0265710c4206463deb937b0c3a4819b69f471a0eca5865719f9" },
    { EmuSC::ControlRom::SynthGen::SC55, "SCC-1",
      "?.xx", "19xx-xx-xx", "1.20",
      "fef1acb1969525d66238be5e7811108919b07a4df5fbab656ad084966373483f" },
    { EmuSC::ControlRom::SynthGen::SC55mk2, "SCC-1A",
      "1.21", "1991-08-10", "1.20",
      "f392335781684976b6f34e89b456cdd0ce874ceeaa3a0c2535837cbb0a6c4a20" },
    { EmuSC::ControlRom::SynthGen::SC55mk2, "SCC-1A",
      "?.xx", "19xx-xx-xx", "1.30",
      "f89442734fdebacae87c7707c01b2d7fdbf5940abae738987aee912d34b5882e" },
    { EmuSC::ControlRom::SynthGen::SC55mk2, "SCB-55",
      "?.xx", "19xx-xx-xx", "2.00",
      "541be4d0b1ef0d07bb042ba67ffd099c8a5d746aac4cd24ce8842c034379f213" },
    { EmuSC::ControlRom::SynthGen::SC55mk2, "SCB-55",
      "?.xx", "19xx-xx-xx", "2.01",
      "e0a3d6d9b05e82374a0d289901273ce560ce1ead86459c75f844158b32d204a9" },
    { EmuSC::ControlRom::SynthGen::SC88, "SC-88",
      "?.xx", "19xx-xx-xx", "3.00",
      "875f561d009fba79296c745b02a83df91105346e292f575d16cf484a17b85be8" },
    { EmuSC::ControlRom::SynthGen::SC88, "SC-88VL",
      "1.04", "1995-07-13", "3.00",
      "712bb3f0cb40f98ef60b491d6c75c077e6ed536474614b61f6a344d9e7f96b0e" }
    }};

  std::array<struct PcmROMInfo, 11> _pcmROMList = {{
      { EmuSC::ControlRom::SynthGen::SC55, "SC-55", "0.20", "19xx-xx-xx",
	{ "5655509a531804f97ea2d7ef05b8fec20ebf46216b389a84c44169257a4d2007",
	  "c655b159792d999b90df9e4fa782cf56411ba1eaa0bb3ac2bdaf09e1391006b1",
	  "334b2d16be3c2362210fdbec1c866ad58badeb0f84fd9bf5d0ac599baf077cc2",
	  "" } },
//      { EmuSC::ControlRom::SynthGen::SC55, "SC-55", "1.20", "19xx-xx-xx",
//	{ "40c093cbfb4441a5c884e623f882a80b96b2527f9fd431e074398d206c0f073d",
//	  "9bbbcac747bd6f7a2693f4ef10633db8ab626f17d3d9c47c83c3839d4dd2f613",
//	  "5b753f6cef4cfc7fcafe1430fecbb94a739b874e55356246a46abe24097ee491",
//	  "" } },
      { EmuSC::ControlRom::SynthGen::SC55mk2, "SC-55mkII", "0.20", "1990-09-12",
	{ "c6429e21b9b3a02fbd68ef0b2053668433bee0bccd537a71841bc70b8874243b",
	  "5b753f6cef4cfc7fcafe1430fecbb94a739b874e55356246a46abe24097ee491",
	  "", "" } }
      }};

};


#endif  // ROM_INFO_H
