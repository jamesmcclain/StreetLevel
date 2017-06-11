/*
 * Source: https://en.wikipedia.org/wiki/Hilbert_curve
 */

#include "curve.h"


#define ONES ((uint32_t)(-1))
#define DOUBLE_TO_BITS(x) (uint32_t)(ONES*x)
#define BITS_TO_DOUBLE(b) (((double)b)/ONES)

void rotate(int bit, uint32_t * x_bits, uint32_t * y_bits, uint64_t rx, uint64_t ry) {
  if (ry == 0) {
    if (rx == 1) {
      *x_bits = (1<<bit) - 1 - *x_bits; // flip across x
      *y_bits = (1<<bit) - 1 - *y_bits; // flip across y
    }

    // flip across diagonal
    uint32_t temp = *x_bits;
    *x_bits = *y_bits;
    *y_bits = temp;
  }
}

uint64_t xy_to_curve(double x, double y) {
  uint32_t x_bits = DOUBLE_TO_BITS(x);
  uint32_t y_bits = DOUBLE_TO_BITS(y);
  uint64_t d = 0;

  // most significant bit to least
  for (int bit = 31; bit >= 0; --bit) {
    uint64_t rx = (x_bits & (1<<bit)) == 0? 0:1;
    uint64_t ry = (y_bits & (1<<bit)) == 0? 0:1;
    d += ((3 * rx) ^ ry) << (bit*2);
    rotate(bit, &x_bits, &y_bits, rx, ry);
  }

  return d;
}

void curve_to_xy(uint64_t d, double * x, double * y) {
  uint32_t x_bits = 0, y_bits = 0;

  // least significant bit to most
  for (int bit = 0; bit <= 31; ++bit) {
    uint64_t rx = 1 & (d>>1);
    uint64_t ry = 1 & (d ^ rx);
    rotate(bit, &x_bits, &y_bits, rx, ry);
    x_bits += rx<<bit;
    y_bits += ry<<bit;
    d >>= 2;
  }

  *x = BITS_TO_DOUBLE(x_bits);
  *y = BITS_TO_DOUBLE(y_bits);
}
