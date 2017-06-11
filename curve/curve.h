#ifndef __CURVE_H__
#define __CURVE_H__

#include <stdint.h>

uint64_t xy_to_curve(double x, double y);
void curve_to_xy(uint64_t d, double * x, double * y);

#endif
