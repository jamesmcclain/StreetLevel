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
#include <stdint.h>
#include <stdlib.h>
#include <immintrin.h>
#include "rasterio.h"


#define ALLOC(P,N) if (!(P = aligned_alloc(PAGESIZE, sizeof(float) * (N)))) { fprintf(stderr, "aligned_alloc failed %s:%d\n", __FILE__, __LINE__); exit(-1); }
#define LASTY(DY) ((int)(y + ((ix + width) * (DY))))
#define BADDIR() { fprintf(stderr, "Bad direction %s:%d\n", __FILE__, __LINE__); exit(-1); }

typedef enum direction {EAST, NORTH, WEST, SOUTH} Direction;


void viewshed_aux(const float * src, float * dst,
                  int cols, int rows,
                  int x, int y, float viewHeight,
                  double xres, double yres,
                  float * alphas, float * dys, float * dms,
                  Direction dir)
{
  int plus_minus;
  int true_cols = cols;

  // If northern or southern wedges, transpose.
  if (dir == SOUTH || dir == NORTH)
    {
      int temp1;
      float temp2;

      temp1 = x, x = y, y = temp1;
      temp1 = cols, cols = rows, rows = temp1;
      temp2 = xres, xres = yres, yres = temp2;
    }

  if (dir == EAST || dir == SOUTH) plus_minus = 1;
  else if (dir == WEST || dir == NORTH) plus_minus = -1;
  else BADDIR();

  for (int j = 0; j < rows; ++j)
    {
      if (dir == EAST || dir == SOUTH) dys[j] = ((float)(j - y))/(cols - x);
      else if (dir == WEST || dir == NORTH) dys[j] = ((float)(j - y))/x;
      else BADDIR();
      dms[j] = sqrt((1*xres)*(1*xres) + (dys[j]*yres)*(dys[j]*yres));
      alphas[j] = -INFINITY;
    }

  for (int i = x, width = 1; (i < cols) && (i > 0); i += plus_minus * width)
    {
      int ix;

      if (dir == EAST || dir == SOUTH) ix = i - x;
      else if (dir == WEST || dir == NORTH) ix = x - i;
      else BADDIR();

      // Compute the width of this slice of columns.  Nominally
      // TILESIZE, but may be something else on the first iteration in
      // order to get aligned with tiles/pages.
      if (i == x && (dir == EAST || dir == SOUTH))
        for (; (i + width) % TILESIZE; ++width);
      else if (i == x && (dir == WEST || dir == NORTH))
        for (; (i - width) % TILESIZE; ++width);
      else width = TILESIZE;

      for (int j = 0; j < rows; ++j) // for each ray (indexed by final row)
        {
          float __attribute__ ((aligned)) dy = dys[j];
          int __attribute__ ((aligned)) last_y = LASTY(dy);

          // Minimize repeated-evaluation of the same pixel by
          // skipping overlapping rays.  If the last y value of this
          // ray-chunk is the same as that of the next ray-chunk, then
          // defer to the latter.  Otherwise, if they are different,
          // evaluate this chunk of the ray.
          if (j == rows-1 || last_y != LASTY(dys[j+1]))
            {
              // restore context from arrays
              float __attribute__ ((aligned)) dm = dms[j];
              float __attribute__ ((aligned)) alpha = alphas[j];
              float __attribute__ ((aligned)) current_y = y + ix * dy;
              float __attribute__ ((aligned)) current_distance = ix * dm;

#if defined(__AVX__) && (TILESIZE == 32) && (REGISTERSIZE == 8)
              if (width == 32)
                {
                  float __attribute__((aligned (32))) elevations[32];
                  float __attribute__((aligned (32))) distances[32];
                  int vanilla_indices[32];

                  // load elevations
                  for (int k = 0; k < 32; ++k, current_y += dy, current_distance += dm)
                    {
                      int current_x, current_y_int;

                      if (dir == EAST)
                        {
                          current_x = i + k;
                          current_y_int = (int)current_y;
                        }
                      else if (dir == SOUTH)
                        {
                          current_y_int = i + k;
                          current_x = (int)current_y;
                        }
                      else if (dir == WEST)
                        {
                          current_x = i - k;
                          current_y_int = (int)current_y;
                        }
                      else if (dir == NORTH)
                        {
                          current_y_int = i - k;
                          current_x = (int)current_y;
                        }
                      else BADDIR();

                      vanilla_indices[k] = xy_to_vanilla_index(true_cols, current_x, current_y_int);
                      elevations[k] = src[xy_to_fancy_index(true_cols, current_x, current_y_int)]; // elevations
                      distances[k] = current_distance; // distances
                    }

                  __asm__ __volatile(
                                     "vbroadcastss 0x0(%0), %%ymm0 \n" // viewHeights
                                     "vmovaps 0x0(%1), %%ymm1 \n" // raw elevations
                                     "vmovaps 0x0(%2), %%ymm2 \n" // distances
                                     "vmovaps 0x20(%1), %%ymm3 \n"
                                     "vmovaps 0x20(%2), %%ymm4 \n"
                                     "vmovaps 0x40(%1), %%ymm5 \n"
                                     "vmovaps 0x40(%2), %%ymm6 \n"
                                     "vmovaps 0x60(%1), %%ymm7 \n"
                                     "vmovaps 0x60(%2), %%ymm8 \n"
                                     "vsubps %%ymm0, %%ymm1, %%ymm1 \n" // elevations
                                     "vsubps %%ymm0, %%ymm3, %%ymm3 \n"
                                     "vsubps %%ymm0, %%ymm5, %%ymm5 \n"
                                     "vsubps %%ymm0, %%ymm7, %%ymm7 \n"
                                     "vdivps %%ymm2, %%ymm1, %%ymm1 \n" // angles
                                     "vdivps %%ymm4, %%ymm3, %%ymm3 \n"
                                     "vdivps %%ymm6, %%ymm5, %%ymm5 \n"
                                     "vdivps %%ymm8, %%ymm7, %%ymm7 \n"
                                     "vmovaps %%ymm1, 0x0(%1) \n" // store angles
                                     "vmovaps %%ymm3, 0x20(%1) \n"
                                     "vmovaps %%ymm5, 0x40(%1) \n"
                                     "vmovaps %%ymm7, 0x60(%1) \n"
                                     :
                                     : "q"(&viewHeight), "q"(elevations), "q"(distances)
                                     : "memory", "%ymm0", "%ymm1", "%ymm2", "%ymm3", "%ymm4", "%ymm5", "%ymm6", "%ymm7", "%ymm8"
                                     );

                  // write into target image
                  for (int k = 0; k < 32; ++k)
                    {
                      float angle = elevations[k];
                      int index = vanilla_indices[k];

                      if (alpha < angle)
                        {
                          alpha = angle;
                          dst[index] = 1.0;
                        }
                    }
                }
              else
#endif
                {
                  for (int k = 0; k < width; ++k, current_y += dy, current_distance += dm)
                    {
                      int current_x;
                      if (dir == EAST || dir == SOUTH) current_x = i + k;
                      else if (dir == WEST || dir == NORTH) current_x = i - k;
                      else BADDIR();

                      int fancy_index;
                      if (dir == EAST || dir == WEST)
                        fancy_index = xy_to_fancy_index(true_cols, current_x, (int)current_y);
                      else if (dir == SOUTH || dir == NORTH)
                        fancy_index = xy_to_fancy_index(true_cols, (int)current_y, current_x);
                      else BADDIR();

                      float elevation = src[fancy_index] - viewHeight;
                      float angle = elevation / current_distance;

                      if (alpha < angle)
                        {
                          int index;
                          if (dir == EAST || dir == WEST)
                            index = xy_to_vanilla_index(true_cols, current_x, (int)current_y);
                          else if (dir == SOUTH || dir == NORTH)
                            index = xy_to_vanilla_index(true_cols, (int)current_y, current_x);
                          else BADDIR();

                          alpha = angle;
                          dst[index] = 1.0;
                        }
                    }
                }

              // save context for this ray and all that overlap it
              for (int k = j; (k >= 0) && (last_y == LASTY(dys[k])); --k) alphas[k] = alpha;
            }
        }
    }
}

void viewshed_cpu(const float * src, float * dst,
                  int cols, int rows,
                  int x, int y, float z,
                  double xres, double yres)
{
  int larger = (cols > rows ? cols : rows);
  float * alphas = NULL;
  float * dys = NULL;
  float * dms = NULL;

  ALLOC(alphas, larger);
  ALLOC(dys, larger);
  ALLOC(dms, larger);

  viewshed_aux(src, dst, cols, rows, x, y, z, xres, yres, alphas, dys, dms, EAST);
  viewshed_aux(src, dst, cols, rows, x, y, z, xres, yres, alphas, dys, dms, SOUTH);
  viewshed_aux(src, dst, cols, rows, x, y, z, xres, yres, alphas, dys, dms, WEST);
  viewshed_aux(src, dst, cols, rows, x, y, z, xres, yres, alphas, dys, dms, NORTH);

  free(alphas);
  free(dys);
  free(dms);
}
