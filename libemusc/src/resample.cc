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

#include "resample.h"

namespace EmuSC {
double interp_coeff_cubic[EMUSC_INTERP_MAX][4];
double interp_coeff_linear[EMUSC_INTERP_MAX][2];

void init_interp_tables() {
  for (int i = 0; i < EMUSC_INTERP_MAX; i++) {
    double x = (double) i / EMUSC_INTERP_MAX;

    // Source: https://github.com/FluidSynth/fluidsynth/blob/master/src/gentables/gen_rvoice_dsp.c
    interp_coeff_cubic[i][0] = (x * (-0.5 + x * (1 - 0.5 * x)));
    interp_coeff_cubic[i][1] = (1.0 + x * x * (1.5 * x - 2.5));
    interp_coeff_cubic[i][2] = (x * (0.5 + x * (2.0 - 1.5 * x)));
    interp_coeff_cubic[i][3] = (0.5 * x * x * (x - 1.0));

    interp_coeff_linear[i][0] = 1.0 - x;
    interp_coeff_linear[i][1] = x;
  }
}
} // namespace EmuSC
