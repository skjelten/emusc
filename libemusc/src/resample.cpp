//
// Created by Matt Montag on 4/15/24.
//

#include "resample.h"
//#include "partial.h"
//
//float emusc_interp_cubic(const Partial *partial, float phase_incr)
//{
//  int index = (int)partial->phase;
//  float fraction = partial->phase - index;
//
//  float a = interp_coeff_cubic[index][0];
//  float b = interp_coeff_cubic[index][1];
//  float c = interp_coeff_cubic[index][2];
//  float d = interp_coeff_cubic[index][3];
//
//  float y = a * partial->samples[0] + b * partial->samples[1] + c * partial->samples[2] + d * partial->samples[3];
//
//  partial->phase += phase_incr;
//
//  return y;
//}
double interp_coeff_cubic[EMUSC_INTERP_MAX][4];
double interp_coeff_linear[EMUSC_INTERP_MAX][2];

void init_interp_tables()
{
  for (int i = 0; i < EMUSC_INTERP_MAX; i++)
  {
    double x = (double)i / EMUSC_INTERP_MAX;
    double x2 = x * x;
    double x3 = x2 * x;

    interp_coeff_cubic[i][0] = 0.5 * (2.0 * x3 - 3.0 * x2 + 1.0);
    interp_coeff_cubic[i][1] = 0.5 * (3.0 * x2 - 2.0 * x3);
    interp_coeff_cubic[i][2] = 0.5 * (x3 - 2.0 * x2 + x);
    interp_coeff_cubic[i][3] = 0.5 * (x3 - x2);

    interp_coeff_linear[i][0] = 1.0 - x;
    interp_coeff_linear[i][1] = x;
  }
}
