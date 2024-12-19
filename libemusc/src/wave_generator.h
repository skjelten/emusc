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


#ifndef __WAVE_GENERATOR_H__
#define __WAVE_GENERATOR_H__


#include "control_rom.h"
#include "settings.h"

#include <array>

#include <stdint.h>


namespace EmuSC {

class WaveGenerator
{
public:
  enum class Waveform {
    Sine        = 0,
    Square      = 1,
    Sawtooth    = 2,
    Triangle    = 3,
    RandomLevel = 8,
    RandomSlope = 9,
  };

  // LFO1 is defined in the Instrument section
  WaveGenerator(struct ControlRom::Instrument &instrument,
                Settings *settings, int partId);

  // LFO2s are defined in the Instrument Partial section
  WaveGenerator(struct ControlRom::InstPartial &instPartial,
                Settings *settings, int partId);
  ~WaveGenerator();

  void next(void);
  inline double value() { return _currentValue; }

private:
  WaveGenerator();

  void _update_params(void);

  bool _id;
  enum Waveform _waveForm;
  int _sampleRate;
  float _sampleFactor;

  int _rate;
  int _rateChange;            // Change in rate due to controller input etc.
  int _delay;                 // Delay time in number of samples
  int _fade;                  // Fade time in number of samples
  int _fadeMax;

  float _currentValue;
  float _random;
  float _randomStart;

  Settings *_settings;
  int _partId;

  bool _useLUT;
  float _index;
  bool _interpolate;               // Linear interpolation

  unsigned int _updateParams = 0;

  static constexpr std::array<float, 128> _delayTable = {
    0.001, 0.003, 0.008, 0.014, 0.021, 0.029, 0.037, 0.045,
    0.054, 0.063, 0.072, 0.081, 0.090, 0.099, 0.108, 0.117,
    0.126, 0.135, 0.144, 0.153, 0.162, 0.171, 0.180, 0.189,
    0.198, 0.207, 0.217, 0.227, 0.237, 0.247, 0.257, 0.267,
    0.277, 0.287, 0.298, 0.310, 0.322, 0.335, 0.349, 0.364,
    0.380, 0.396, 0.413, 0.430, 0.447, 0.464, 0.481, 0.498,
    0.515, 0.532, 0.549, 0.566, 0.583, 0.601, 0.619, 0.637,
    0.655, 0.673, 0.691, 0.709, 0.727, 0.745, 0.763, 0.781,
    0.799, 0.817, 0.835, 0.853, 0.871, 0.889, 0.907, 0.925,
    0.943, 0.961, 0.979, 0.997, 1.015, 1.033, 1.051, 1.069,
    1.088, 1.108, 1.129, 1.151, 1.174, 1.198, 1.224, 1.251,
    1.280, 1.310, 1.341, 1.373, 1.406, 1.441, 1.477, 1.515,
    1.555, 1.598, 1.644, 1.693, 1.746, 1.804, 1.868, 1.940,
    2.020, 2.109, 2.208, 2.318, 2.440, 2.574, 2.721, 2.881,
    3.055, 3.243, 3.446, 3.664, 3.898, 4.148, 4.415, 4.699,
    5.001, 5.322, 5.667, 6.047, 6.507, 7.107, 8.000, 10.000 };

    static constexpr std::array<float, 256> _sineTable = {
    0, 0.0245412, 0.0490677, 0.0735646, 0.0980171, 0.122411, 0.14673,
    0.170962, 0.19509, 0.219101, 0.24298, 0.266713, 0.290285, 0.313682, 0.33689,
    0.359895, 0.382683, 0.405241, 0.427555, 0.449611, 0.471397, 0.492898, 0.514103,
    0.534998, 0.55557, 0.575808, 0.595699, 0.615232, 0.634393, 0.653173, 0.671559,
    0.689541, 0.707107, 0.724247, 0.740951, 0.757209, 0.77301, 0.788346, 0.803208,
    0.817585, 0.83147, 0.844854, 0.857729, 0.870087, 0.881921, 0.893224, 0.903989,
    0.91421, 0.92388, 0.932993, 0.941544, 0.949528, 0.95694, 0.963776, 0.970031,
    0.975702, 0.980785, 0.985278, 0.989177, 0.99248, 0.995185, 0.99729, 0.998795,
    0.999699, 1, 0.999699, 0.998795, 0.99729, 0.995185, 0.99248, 0.989177, 0.985278,
    0.980785, 0.975702, 0.970031, 0.963776, 0.95694, 0.949528, 0.941544, 0.932993,
    0.92388, 0.91421, 0.903989, 0.893224, 0.881921, 0.870087, 0.857729, 0.844854,
    0.83147, 0.817585, 0.803208, 0.788346, 0.77301, 0.757209, 0.740951, 0.724247,
    0.707107, 0.689541, 0.671559, 0.653173, 0.634393, 0.615232, 0.595699, 0.575808,
    0.55557, 0.534998, 0.514103, 0.492898, 0.471397, 0.449611, 0.427555, 0.405241,
    0.382683, 0.359895, 0.33689, 0.313682, 0.290285, 0.266713, 0.24298, 0.219101,
    0.19509, 0.170962, 0.146731, 0.122411, 0.0980171, 0.0735644, 0.0490677, 0.0245412,
    -8.74228e-08, -0.0245411, -0.0490677, -0.0735646, -0.098017, -0.122411, -0.14673, -0.170962,
    -0.19509, -0.219101, -0.24298, -0.266713, -0.290285, -0.313682, -0.33689, -0.359895,
    -0.382683, -0.405241, -0.427555, -0.449611, -0.471397, -0.492898, -0.514103, -0.534998,
    -0.55557, -0.575808, -0.595699, -0.615232, -0.634393, -0.653173, -0.671559, -0.689541,
    -0.707107, -0.724247, -0.740951, -0.757209, -0.77301, -0.788346, -0.803208, -0.817585,
    -0.831469, -0.844853, -0.857729, -0.870087, -0.881921, -0.893224, -0.903989, -0.91421,
    -0.92388, -0.932993, -0.941544, -0.949528, -0.95694, -0.963776, -0.970031, -0.975702,
    -0.980785, -0.985278, -0.989177, -0.99248, -0.995185, -0.99729, -0.998795, -0.999699,
    -1, -0.999699, -0.998795, -0.99729, -0.995185, -0.99248, -0.989177, -0.985278,
    -0.980785, -0.975702, -0.970031, -0.963776, -0.95694, -0.949528, -0.941544, -0.932993,
    -0.923879, -0.91421, -0.903989, -0.893224, -0.881921, -0.870087, -0.857729, -0.844853,
    -0.83147, -0.817585, -0.803208, -0.788346, -0.77301, -0.757209, -0.740951, -0.724247,
    -0.707107, -0.689541, -0.671559, -0.653173, -0.634393, -0.615231, -0.595699, -0.575808,
    -0.55557, -0.534998, -0.514103, -0.492898, -0.471397, -0.449612, -0.427555, -0.405241,
    -0.382683, -0.359895, -0.33689, -0.313682, -0.290285, -0.266713, -0.24298, -0.219101,
    -0.19509, -0.170962, -0.14673, -0.122411, -0.0980172, -0.0735646, -0.0490676, -0.0245411 };

};

}

#endif  // __WAVE_GENERATOR_H__
