/*
 * Copyright (c) 2010-2014 and 2016-2017, James McClain and Mark Pugner
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

#include <stdio.h>
#include <stdlib.h>

#include "ansi.h"
#include "curve/curve_interface.h"
#include "index/index.h"
#include "pdal/pdal_point.h"

#define MIN(a,b) ((a)<(b)?(a):(b))
#define MAX(a,b) ((a)<(b)?(b):(a))
#define EPSILON (0.001)

typedef struct query_key {
  uint64_t key;
  const pdal_point ** point;
} query_key;


int compar(const void * _key, const void * _point) {
  const query_key * key = _key;
  const pdal_point * point = _point;

  *(key->point) = point;
  if (key->key < point->key) return -1;
  else if (key->key > point->key) return +1;
  else return 0;
}

int main(int argc, const char ** argv)
{
  void * _data;
  pdal_point * data;
  char * projection;
  double x_min, x_max, y_min, y_max;
  unsigned long long sample_count;
  struct stat stat;

  if (argc < 3) {
    fprintf(stderr, "Usage: %s <so_filename> <index_filename>\n", argv[0]);
    exit(-1);
  }

  load_curve(argv[1]);
  _data = map_index(argv[2], &stat);

  data = read_header(_data,
                     curve_name(), curve_version(),
                     &projection,
                     &x_min, &x_max, &y_min, &y_max,
                     &sample_count);

  fprintf(stdout,
          ANSI_COLOR_CYAN "projection = "
          ANSI_COLOR_MAGENTA "%s"
          ANSI_COLOR_RESET "\n",
          projection);
  fprintf(stdout,
          ANSI_COLOR_CYAN "bounding box = "
          ANSI_COLOR_MAGENTA "%lf %lf %lf %lf"
          ANSI_COLOR_RESET "\n",
          x_min, x_max, y_min, y_max);
  fprintf(stdout,
          ANSI_COLOR_CYAN "sample count = "
          ANSI_COLOR_MAGENTA "%lld"
          ANSI_COLOR_RESET "\n",
          sample_count);
  fprintf(stdout, "\n");

  /* Query */
  {
    pdal_point point = data[33];
    double bb_min_x = MAX(point.x - EPSILON, 0.0);
    double bb_min_y = MAX(point.y - EPSILON, 0.0);
    double bb_max_x = MIN(point.x + EPSILON, 1.0);
    double bb_max_y = MIN(point.y + EPSILON, 1.0);
    const pdal_point * min_point = NULL, * max_point = NULL;
    query_key min_key = {.point = &min_point, .key = xy_to_curve(bb_min_x, bb_min_y)};
    query_key max_key = {.point = &max_point, .key = xy_to_curve(bb_max_x, bb_max_y)};
    double x, y;
    uint32_t x_bits1, y_bits1, x_bits2, y_bits2;

    fprintf(stdout,
            ANSI_COLOR_BLUE "query point:\t"
            ANSI_COLOR_GREEN "key = 0x%016lX\t x = %lf\t y = %lf\t z = %lf"
            ANSI_COLOR_RESET "\n",
            point.key, point.x, point.y, point.z);

    curve_to_xy(min_key.key, &x, &y);
    x_bits1 = (uint32_t)(((uint32_t)(-1))*x);
    y_bits1 = (uint32_t)(((uint32_t)(-1))*y);
    fprintf(stdout,
            ANSI_COLOR_BLUE "min_key:\t"
            ANSI_COLOR_GREEN "key = 0x%016lX\t x = %lf\t y = %lf\t x_bits = 0x%08X\t y_bits = 0x%08X"
            ANSI_COLOR_RESET "\n",
            min_key.key, x, y, x_bits1, y_bits1);

    curve_to_xy(max_key.key, &x, &y);
    x_bits2 = (uint32_t)(((uint32_t)(-1))*x);
    y_bits2 = (uint32_t)(((uint32_t)(-1))*y);
    fprintf(stdout,
            ANSI_COLOR_BLUE "max_key:\t"
            ANSI_COLOR_GREEN "key = 0x%016lX\t x = %lf\t y = %lf\t x_bits = 0x%08X\t y_bits = 0x%08X"
            ANSI_COLOR_RESET "\n",
            max_key.key, x, y, x_bits2, y_bits2);

    /* fprintf(stdout, */
    /*         ANSI_COLOR_BLUE "XOR:\t\t\t\t\t\t\t\t\t\t" */
    /*         ANSI_COLOR_GREEN " x_xor  = 0x%08X\t y_xor  = 0x%08X" */
    /*         ANSI_COLOR_RESET "\n", */
    /*         (x_bits1 ^ x_bits2), (y_bits1 ^ y_bits2)); */

    bsearch(&min_key, data, sample_count, sizeof(pdal_point), compar);
    fprintf(stdout,
            ANSI_COLOR_BLUE "min_point:\t"
            ANSI_COLOR_GREEN "key = 0x%016lX\t x = %lf\t y = %lf\t z = %lf\t index = %ld"
            ANSI_COLOR_RESET "\n",
            min_point->key, min_point->x, min_point->y, min_point->z, min_point - data);

    bsearch(&max_key, data, sample_count, sizeof(pdal_point), compar);
    fprintf(stdout,
            ANSI_COLOR_BLUE "max_point:\t"
            ANSI_COLOR_GREEN "key = 0x%016lX\t x = %lf\t y = %lf\t z = %lf\t index = %ld"
            ANSI_COLOR_RESET "\n",
            max_point->key, max_point->x, max_point->y, max_point->z, max_point - data);
  }

  unmap_index(_data, &stat);
}
