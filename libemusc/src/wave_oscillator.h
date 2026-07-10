/*
 *  This file is part of libEmuSC, a Sound Canvas emulator library
 *  Copyright (C) 2022-2026  Håkon Skjelten
 *
 *  libEmuSC is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU Lesser General Public License as published
 *  by the Free Software Foundation, either version 2.1 of the License, or
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

// The wavetable oscillator reads the sample sets and uses a simple linear
// interpolation. To save runtime CPU usage EmuSC pre-decodes all the DPCM
// samples to sample sets stored in vectors. To make this work with ping-pong
// loops, the sample sets have been extended to include the return path so
// they basically becomes forward loops.

// The interpolation lookup table consists of 3 x 128 entries, Q12 fixed point.
// These are the cumulative delta weights used by the SC-55 external PCM chip:
//     y(t) = s0 + c0(t)*(s1-s0) + c1(t)*(s2-s1) + c2(t)*(s3-s2)
// where s0 is the sample at the current integer position, s1..s3 the next
// three samples in the direction of travel, and t the fractional phase.
// The interpolation algorithm and lookup table is is based on information from
// the Nuked-SC55 project by nukeykt (https://github.com/nukeykt/Nuked-SC55).


#ifndef __WAVE_OSCILLATOR_H__
#define __WAVE_OSCILLATOR_H__


#include "control_rom.h"
#include "pitch.h"

#include <cstdint>
#include <functional>
#include <vector>


namespace EmuSC {


class WaveOscillator
{
public:
  WaveOscillator(ControlRom::Sample *ctrlSample, std::vector<float> *pcmSamples,
                 std::function<void(void)> cb);

  void get_sample_set(Pitch *pitch, float pitchBend,
                      std::array<float, 256> &dryBus);

private:
  int _sampleStart;           // 0 or portamento offset if portamento is active
  int _sampleEnd;             // Sample set length
  int _loopStart;             // _sampleEnd - sample set loop length
  int _loopLength;            // Sample set loop length

  std::vector<float> *_pcmSamples;

  float _phase;               // Phase fraction 0.0 - 1.0
  int _index;                 // Integer index

  enum class LoopMode {
    Forward  = 0,
    PingPong = 1,
    NoLoop   = 2
  };
  enum LoopMode _loopMode;

  // External function call for signaling end of first run of sample set
  std::function<void(void)> _firstRunCompleteCallback = NULL;
  bool _firstRunComplete;

  float _fetch_sample(int index);
  float _interpolate();

  // Lookup table from the Nuked-SC55 project by nukeykt
  static constexpr uint16_t _interpolationLUT[3][128] = {
    // c0 (cumulative weight of delta0)
    3385, 3401, 3417, 3432, 3448, 3463, 3478, 3492,
    3506, 3521, 3534, 3548, 3562, 3575, 3588, 3601,
    3614, 3626, 3638, 3650, 3662, 3673, 3685, 3696,
    3707, 3718, 3728, 3739, 3749, 3759, 3768, 3778,
    3787, 3796, 3805, 3814, 3823, 3831, 3839, 3847,
    3855, 3863, 3870, 3878, 3885, 3892, 3899, 3905,
    3912, 3918, 3924, 3930, 3936, 3942, 3948, 3953,
    3958, 3963, 3968, 3973, 3978, 3983, 3987, 3991,
    3995, 4000, 4004, 4007, 4011, 4015, 4018, 4022,
    4025, 4028, 4031, 4034, 4037, 4040, 4042, 4045,
    4047, 4050, 4052, 4054, 4057, 4059, 4061, 4063,
    4064, 4066, 4068, 4070, 4071, 4073, 4074, 4076,
    4077, 4078, 4079, 4081, 4082, 4083, 4084, 4085,
    4086, 4086, 4087, 4088, 4089, 4089, 4090, 4091,
    4091, 4092, 4092, 4093, 4093, 4094, 4094, 4094,
    4094, 4095, 4095, 4095, 4095, 4095, 4095, 4095,

    // c1 (cumulative weight of delta1)
    710,  726,  742,  758,  775,  792,  809,  826,
    844,  861,  879,  897,  915,  933,  952,  971,
    990, 1009, 1028, 1047, 1067, 1087, 1106, 1126,
    1147, 1167, 1188, 1208, 1229, 1250, 1271, 1292,
    1314, 1335, 1357, 1379, 1400, 1423, 1445, 1467,
    1489, 1512, 1534, 1557, 1580, 1602, 1625, 1648,
    1671, 1695, 1718, 1741, 1764, 1788, 1811, 1835,
    1858, 1882, 1906, 1929, 1953, 1977, 2000, 2024,
    2048, 2069, 2095, 2119, 2143, 2166, 2190, 2214,
    2237, 2261, 2284, 2308, 2331, 2355, 2378, 2401,
    2425, 2448, 2471, 2494, 2517, 2539, 2562, 2585,
    2607, 2630, 2652, 2674, 2696, 2718, 2740, 2762,
    2783, 2805, 2826, 2847, 2868, 2889, 2910, 2931,
    2951, 2971, 2991, 3011, 3031, 3051, 3070, 3089,
    3108, 3127, 3146, 3164, 3182, 3200, 3218, 3236,
    3253, 3271, 3288, 3304, 3321, 3338, 3354, 3370,

    // c2 (cumulative weight of delta2)
     0,    0,    0,    1,    1,    1,    2,    2,
     3,    3,    3,    4,    4,    5,    5,    6,
     6,    7,    8,    8,    9,   10,   10,   11,
    12,   13,   14,   15,   16,   17,   18,   19,
    20,   22,   23,   24,   26,   27,   29,   30,
    32,   34,   36,   38,   40,   42,   44,   46,
    49,   51,   53,   56,   59,   62,   65,   68,
    71,   74,   77,   81,   84,   88,   92,   96,
   100,  104,  109,  113,  118,  122,  127,  132,
   137,  143,  148,  154,  160,  165,  171,  178,
   184,  191,  197,  204,  211,  219,  226,  234,
   241,  249,  257,  266,  274,  283,  292,  301,
   310,  319,  329,  339,  349,  359,  369,  380,
   391,  402,  413,  424,  436,  448,  460,  472,
   484,  497,  510,  523,  536,  549,  563,  577,
   591,  605,  619,  634,  648,  663,  679,  694
  };
};

}

#endif // __WAVE_OSCILLATOR_H__
