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

#ifndef EMUSC_RESAMPLE_H
#define EMUSC_RESAMPLE_H

#define EMUSC_INTERP_MAX 256
#define emusc_float_to_row(_x) ((unsigned int)((_x) * EMUSC_INTERP_MAX))

// Interpolation modes
enum class InterpMode {
  Nearest,
  Linear,
  Cubic
};

namespace EmuSC {
extern double interp_coeff_cubic[EMUSC_INTERP_MAX][4];
extern double interp_coeff_linear[EMUSC_INTERP_MAX][2];

void init_interp_tables();
} // namespace EmuSC

#endif //EMUSC_RESAMPLE_H
