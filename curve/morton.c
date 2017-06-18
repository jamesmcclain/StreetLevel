/*
 * Copyright (c) 2017, James McClain and Mark Pugner
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the
 *    distribution.
 * 3. All advertising materials mentioning features or use of this
 *    software must display the following acknowledgement: This product
 *    includes software developed by Dr. James W. McClain and Dr. Mark
 *    C. Pugner.
 * 4. Neither the names of the authors nor the names of the
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS ''AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdint.h>

#define ONES ((uint32_t)(-1))
#define DOUBLE_TO_BITS(x) (uint32_t)(ONES*x)
#define BITS_TO_DOUBLE(b) (((double)b)/ONES)


uint64_t xy_to_curve(double x, double y) {
  uint32_t x_bits = DOUBLE_TO_BITS(x);
  uint32_t y_bits = DOUBLE_TO_BITS(y);
  uint64_t d = 0;

  for (int bit = 31; bit >= 0; --bit) {
    uint64_t rx = (x_bits & (1<<bit)) == 0? 0:1;
    uint64_t ry = (y_bits & (1<<bit)) == 0? 0:1;
    d |= (rx<<(2*bit+1));
    d |= (ry<<(2*bit+0));
  }

  return d;
}

void curve_to_xy(uint64_t d, double * x, double * y) {
  uint32_t x_bits = 0, y_bits = 0;

  for (int bit = 31; bit >= 0; --bit) {
    uint32_t rx = (d & (((uint64_t)1)<<(bit*2+1))) == 0? 0:1;
    uint32_t ry = (d & (((uint64_t)1)<<(bit*2+0))) == 0? 0:1;
    x_bits |= (rx<<bit);
    y_bits |= (ry<<bit);
  }

  *x = BITS_TO_DOUBLE(x_bits);
  *y = BITS_TO_DOUBLE(y_bits);
}

char * curve_name() {
  return "morton";
}

uint32_t curve_version() {
  return 0;
}
