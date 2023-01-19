/*  
 *  This file is part of libEmuSC, a Sound Canvas emulator library
 *  Copyright (C) 2022  Håkon Skjelten
 *
 *  libEmuSC is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU Lesser General Public License as published
 *  by the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  libEmuSC is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with libEmuSC. If not, see <http://www.gnu.org/licenses/>.
 */

// NOTES:
// Based on Kitrinx's research it seems like the static base filter is replaced
// by the TVF envelope when present, and not combined as is quite common.
// Base filter is reportedly ranging from ~500Hz at 1, to 'fully open' at 127.
// It is also noted that the SC-55 seemingly suffer from hardware malfunction
// if both the base filer and TVF envelope is set to 0 for a partial.

// Based on the owner's manuals and online reviews is seems like the SC-55 and
// SC-88 are based on low-pass filter only in their TVF.


#include "tvf.h"

#include <cmath>
#include <iostream>
#include <string.h>

namespace EmuSC {


TVF::TVF(ControlRom::InstPartial &instPartial, uint8_t key, Settings *settings,
	 int8_t partId)
  : _sampleRate(settings->get_param_uint32(SystemParam::SampleRate)),
    _lpFilter(NULL),
    _ahdsr(NULL)
{
  /*
  double phaseLevel[5];         // Phase volume for phase 1-5
  double phaseDuration[5];      // Phase duration for phase 1-5
  bool   phaseShape[5];         // Phase shape for phase 1-5

  // Set adjusted values for volume (0-127) and time (seconds)
  phaseLevel[0] = _convert_volume(instPartial.TVFLvlP1);
  phaseLevel[1] = _convert_volume(instPartial.TVFLvlP2);
  phaseLevel[2] = _convert_volume(instPartial.TVFLvlP3);
  phaseLevel[3] = _convert_volume(instPartial.TVFLvlP4);
  phaseLevel[4] = 0;

  phaseDuration[0] = _convert_time_to_sec(instPartial.TVFDurP1 & 0x7F);
  phaseDuration[1] = _convert_time_to_sec(instPartial.TVFDurP2 & 0x7F);
  phaseDuration[2] = _convert_time_to_sec(instPartial.TVFDurP3 & 0x7F);
  phaseDuration[3] = _convert_time_to_sec(instPartial.TVFDurP4 & 0x7F);
  phaseDuration[4] = 0; //_convert_time_to_sec(instPartial.TVFDurP5 & 0x7F);

  phaseShape[0] = (instPartial.TVFDurP1 & 0x80) ? 0 : 1;
  phaseShape[1] = (instPartial.TVFDurP2 & 0x80) ? 0 : 1;
  phaseShape[2] = (instPartial.TVFDurP3 & 0x80) ? 0 : 1;
  phaseShape[3] = (instPartial.TVFDurP4 & 0x80) ? 0 : 1;
  phaseShape[4] = 0; //(instPartial.TVFDurP5 & 0x80) ? 0 : 1;

//  _ahdsr = new AHDSR(phaseLevel, phaseDuration, phaseShape, sampleRate);
//  _ahdsr->start();
*/
  // TODO: Add calculations for TVF cutoff freq. and resonance
  // -> partSettings[(int) SysExPart::TVFcutofFreq] - 0x40)
  // -> partSettings[(int) SysExPart::TVFresonance] - 0x40)

  // TODO: Add envelope adjustments TVF attack, decay and release
  // -> partSettings[(int) SysExPart::TVFAenvAttack]  - 0x40)
  // -> partSettings[(int) SysExPart::TVFAenvDecay]   - 0x40)
  // -> partSettings[(int) SysExPart::TVFAenvRelease] - 0x40)

  // TODO: Replace this grossly approximate linear base frequency with a proper
  //       function or LUT
  _lpBaseFrequency = instPartial.TVFBaseFlt * 110;

  if (!_lpBaseFrequency)
    return;

  // TODO: Figure out if the Sound Canvas has cutoff key follow on base filter
  //       Assuming cutoff key follow with C4 (note 60) as 0 point
  if (_lpBaseFrequency && key < 60)
    _lpBaseFrequency -= (60 - key) * 2^(1/12);
  else if (_lpBaseFrequency)
    _lpBaseFrequency += (key - 60) * 2^(1/12);

  // TODO: Figure out correct min / max resonance
  //       Assuming ROM data spans at linear rate between 0.7 and 8.7
  //       0.707 + (ROM / 128 * 8)
  _lpResonance = 0.707 + instPartial.TVFResonance / 16.0;

  _lpFilter = new LowPassFilter(_sampleRate);
  _lpFilter->calculate_coefficients(_lpBaseFrequency, _lpResonance);

  if (0)
    std::cout << "TVF (" << (int) key << "): freq=" << _lpBaseFrequency
	      << " res=" << _lpResonance << std::endl;
}


TVF::~TVF()
{
  if (_ahdsr)
    delete _ahdsr;

  if (_lpFilter)
    delete _lpFilter;
}


double TVF::_convert_time_to_sec(uint8_t time)
{
  if (time == 0)
    return 0;

  return (pow(2.0, (double)(time) / 18.0) / 5.45 - 0.183);
}


double TVF::apply(double input)
{
  double output;


  // TODO: Add TVF envelope
  if (_ahdsr) {
//    return _ahdsr->get_next_value();

  } else if (_lpBaseFrequency) {
    output = _lpFilter->apply(input);

  } else {
    output = input;
  }

  if (0)
    std::cout << "TVF: freq=" << _lpBaseFrequency
	      << " res(Q)=" << _lpResonance << " => " << output << std::endl;

  return output;
}


void TVF::note_off()
{
  _ahdsr->release();
}

}
