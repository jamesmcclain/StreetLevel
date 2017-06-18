#ifndef __CURVE_H__
#define __CURVE_H__

#include <stdint.h>

typedef uint64_t (* to_curve)(double,double);
typedef void (* from_curve)(uint64_t,double *, double *);
typedef char * (* name_of_curve)();
typedef uint32_t (* version_of_curve)();

#endif
