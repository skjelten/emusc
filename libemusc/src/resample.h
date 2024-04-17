//
// Created by Matt Montag on 4/15/24.
//

#ifndef EMUSC_RESAMPLE_H
#define EMUSC_RESAMPLE_H

#define EMUSC_INTERP_BITS 8
#define EMUSC_INTERP_BITS_MASK 0xff000000
#define EMUSC_INTERP_BITS_SHIFT 24
#define EMUSC_INTERP_MAX 256
/*
// 64 bit fractional phase not implemented yet.
#define EMUSC_FRACT_MAX ((double)4294967296.0)
#define emusc_fract_to_row(_x) ((unsigned int)(emusc_phase_fract(_x) & EMUSC_INTERP_BITS_MASK) >> EMUSC_INTERP_BITS_SHIFT)
*/
#define emusc_float_to_row(_x) ((unsigned int)((_x) * EMUSC_INTERP_MAX))


void init_interp_tables();

#endif //EMUSC_RESAMPLE_H
