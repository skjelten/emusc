/*  
 *  This file is part of libEmuSC, a Sound Canvas emulator library
 *  Copyright (C) 2022-2024  Håkon Skjelten
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


#ifndef __TVA_H__
#define __TVA_H__


#include "ahdsr.h"
#include "control_rom.h"
#include "settings.h"
#include "wave_generator.h"

#include <stdint.h>

#include <array>


namespace EmuSC {


class TVA
{
public:
  TVA(ControlRom::InstPartial &instPartial, uint8_t key,
      ControlRom::Sample *ctrlSample, WaveGenerator *LFO1, WaveGenerator *LFO2,
      Settings *settings, int8_t partId, int instVolAtt);
  ~TVA();

  void update_dynamic_params(bool reset = false);

  void apply(double *sample);
  float get_current_value()
  { if (_ahdsr) return _ahdsr->get_current_value(); return 0; }

  void note_off();
  bool finished(void);

private:
  uint32_t _sampleRate;

  WaveGenerator *_LFO1;
  WaveGenerator *_LFO2;

  uint8_t _accLFO1Depth;
  uint8_t _accLFO2Depth;

  uint8_t _key;
  int _drumSet;

  uint8_t _panpot;
  bool _panpotLocked;

  AHDSR *_ahdsr;
  bool _finished;
  
  ControlRom::InstPartial &_instPartial;

  Settings *_settings;
  int8_t _partId;

  // Dynamic volume controls
  float _drumVol;
  float _ctrlVol;

  double _staticVolCorr;

  TVA();

  void _set_static_params(ControlRom::Sample *ctrlSample, int instVolAtt);
  void _init_envelope(void);

  static constexpr std::array<float, 128> _convert_volume_LUT = {
    0,        0.001906, 0.003848, 0.005827, 0.007844, 0.009900, 0.011995,
    0.014129, 0.016305, 0.018522, 0.020781, 0.023083, 0.025429, 0.027820,
    0.030256, 0.032739, 0.035269, 0.037847, 0.040475, 0.043152, 0.045881,
    0.048661, 0.051495, 0.054382, 0.057325, 0.060324, 0.063380, 0.066494,
    0.069667, 0.072901, 0.076197, 0.079555, 0.082978, 0.086465, 0.090019,
    0.093641, 0.097332, 0.101093, 0.104926, 0.108832, 0.112813, 0.116869,
    0.121003, 0.125215, 0.129508, 0.133883, 0.138340, 0.142883, 0.147513,
    0.152231, 0.157038, 0.161938, 0.166930, 0.172018, 0.177203, 0.182486,
    0.187871, 0.193358, 0.198949, 0.204648, 0.210454, 0.216372, 0.222402,
    0.228547, 0.234809, 0.241191, 0.247694, 0.254322, 0.261075, 0.267957,
    0.274971, 0.282118, 0.289401, 0.296824, 0.304387, 0.312095, 0.319950,
    0.327954, 0.336111, 0.344424, 0.352895, 0.361527, 0.370324, 0.379289,
    0.388424, 0.397734, 0.407221, 0.416889, 0.426741, 0.436781, 0.447012,
    0.457439, 0.468064, 0.478891, 0.489925, 0.501170, 0.512628, 0.524305,
    0.536205, 0.548331, 0.560689, 0.573282, 0.586115, 0.599193, 0.612520,
    0.626101, 0.639940, 0.654044, 0.668417, 0.683063, 0.697989, 0.713199,
    0.728699, 0.744494, 0.760591, 0.776994, 0.793710, 0.810744, 0.828104,
    0.845794, 0.863821, 0.882192, 0.900913, 0.919991, 0.939433, 0.959245,
    0.979434, 1
  }; // 0.1 * 2^(volume / 36.7111) - 0.1

};

}

#endif  // __TVA_H__
