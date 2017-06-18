#ifndef __CURVE_INTERFACE_H__
#define __CURVE_INTERFACE_H__

#include "curve.h"

extern to_curve xy_to_curve;
extern from_curve curve_to_xy;
extern name_of_curve curve_name;
extern version_of_curve curve_version;
extern void load_curve(const char * sofilename);

#endif
