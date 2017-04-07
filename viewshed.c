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
#define LASTY(DY) ((int)(y + ((i - x + width) * (DY))))

typedef float v8sf __attribute__ ((vector_size (sizeof(float) * REGISTERSIZE)));
typedef int v8si __attribute__ ((vector_size (sizeof(int) * REGISTERSIZE)));
/* typedef __m256 v8sf; */
/* typedef __m256 v8si; */

#ifndef __AVX__
#define __AVX__ (0)
#endif


void viewshed(const float * src, float * dst,
	      uint32_t cols, uint32_t rows,
	      double xres, double yres)
{
  float * alphas = NULL;
  float * dys = NULL;
  float * dms = NULL;
  int larger = (cols > rows ? cols : rows);

  int x = 4608; // XXX
  int y = 3072; // XXX
  float viewHeight = 2000; // XXX

  ALLOC(alphas, larger);
  ALLOC(dys, larger);
  ALLOC(dms, larger);

  v8sf vmultipliers;
  for (int k = 0; k < REGISTERSIZE; ++k) vmultipliers[k] = (float)k;

  // East
  for (int j = 0; j < larger; ++j)
    {
      dys[j] = ((float)(j - y))/x;
      dms[j] = sqrt((1*xres)*(1*xres) + (dys[j]*yres)*(dys[j]*yres));
      alphas[j] = -INFINITY;
    }
  for (int i = x, width = 1; i < cols; i += width)
    {
      // Compute the width of this slice of columns.  Nominally
      // REGISTERSIZE, but may be something else on the first iteration in
      // order to get aligned with tiles/pages.
      if (i == x)
	for (; (i + width) % REGISTERSIZE; ++width);
      else
	width = REGISTERSIZE;

      for (int j = 0; j < rows; ++j) // for each ray (indexed by final row)
	{
	  float dy = dys[j];
	  int last_y = LASTY(dy);

	  // Minimize repeatedly-evaluating the same pixel by skipping
	  // overlapping rays.  If the last y value of this ray-chunk
	  // is the same as that of the next ray-chunk, then defer to
	  // the latter.  Otherwise, if they are different, evaluate
	  // this chunk of rays.
	  if (j == rows-1 || last_y != LASTY(dys[j+1]))
	    {
	      // restore context from arrays
	      float dm = dms[j];
	      float alpha = alphas[j];
	      float current_y = y + (i - x) * dy;
	      float distance = (i - x) * dm;

	      // Extend ray to the East.  In the common case, use
	      // vectors ...
	      if (width == REGISTERSIZE && __AVX__)
		{
		  v8sf vcurrent_y;
		  v8sf vdistance;
		  v8sf velevation;
		  v8sf vangle;
		  v8si vindex;

		  vcurrent_y = __builtin_ia32_addps256(__builtin_ia32_mulps256(vmultipliers, __builtin_ia32_vbroadcastss256(&dy)), __builtin_ia32_vbroadcastss256(&current_y));
		  vdistance = __builtin_ia32_addps256(__builtin_ia32_mulps256(vmultipliers, __builtin_ia32_vbroadcastss256(&dm)), __builtin_ia32_vbroadcastss256(&distance));
		  for (int k = 0; k < REGISTERSIZE; ++k)
		    {
		      vindex[k] = xy_to_index(cols, (i + k), (int)vcurrent_y[k]);
		      velevation[k] = src[vindex[k]];
		    }
		  velevation = __builtin_ia32_subps256(velevation, __builtin_ia32_vbroadcastss256(&viewHeight));
		  vangle = __builtin_ia32_divps256(velevation, vdistance);
		  for (int k = 0; k < REGISTERSIZE; ++k)
		    {
		      if (alpha < vangle[k])
			{
			  alpha = vangle[k];
			  dst[vindex[k]] = 1.0;
			}
		    }
		}
	      // ... in the special case, use scalers.
	      else
		{
		  for (int k = 0; k < width; ++k, current_y += dy, distance += dm)
		    {
		      int current_x = i + k;
		      int index = xy_to_index(cols, current_x, (int)current_y);
		      float elevation = src[index] - viewHeight;
		      float angle = elevation / distance;

		      if (alpha < angle)
			{
			  alpha = angle;
			  dst[index] = 1.0;
			}
		    }
		}
	      
	      // save context for this ray and all that overlap it
	      for (int k = j; last_y == LASTY(dys[k]); --k)
	      	alphas[k] = alpha;
	    }
	}
    }

  /* for (int i = 0; i < rows; ++i) */
  /*   { */
  /*     float dy, current_y, alpha; */

  /*     // east */
  /*     dy = ((float)(i - y))/x; */
  /*     current_y = y; */
  /*     alpha = -INFINITY; */
  /*     for (int j = x; j < cols; j += TILESIZE) */
  /* 	{ */
  /* 	  for (int k = 0; k < TILESIZE; ++k) */
  /* 	    { */
  /* 	    } */
  /* 	} */
      
  /*     for (int current_x = x; current_x < cols; ++current_x, current_y += dy) */
  /* 	{ */
  /* 	  int index = xy_to_index(cols, current_x, (int)current_y); */
  /* 	  float xchange = xres * (current_x - x); */
  /* 	  float ychange = yres * (current_y - y); */
  /* 	  float distance = sqrt(xchange*xchange + ychange*ychange); */
  /* 	  float elevation = src[index] - viewHeight; */
  /* 	  float angle = atan(elevation / distance); */

  /* 	  if (alpha <= angle) */
  /* 	    { */
  /* 	      alpha = angle; */
  /* 	      dst[index] = 1.0; */
  /* 	    } */
  /* 	} */

      /* // west */
      /* dy = ((float)(i - y))/(cols - x); */
      /* current_y = y; */
      /* alpha = -INFINITY; */
      /* for (int current_x = x; current_x >= 0; --current_x, current_y += dy) */
      /* 	{ */
      /* 	  int index = xy_to_index(cols, current_x, (int)current_y); */
      /* 	  float xchange = xres * (current_x - x); */
      /* 	  float ychange = yres * (current_y - y); */
      /* 	  float distance = sqrt(xchange*xchange + ychange*ychange); */
      /* 	  float elevation = src[index] - viewHeight; */
      /* 	  float angle = atan(elevation / distance); */

      /* 	  if (alpha <= angle) */
      /* 	    { */
      /* 	      alpha = angle; */
      /* 	      dst[index] = 1.0; */
      /* 	    } */
      /* 	} */
    /* } */
  
  /* for (int i = 0; i < cols; ++i) */
  /*   { */
  /*     float dx, current_x, alpha; */
      
  /*     // north */
  /*     dx = ((float)(i - x))/y; */
  /*     current_x = x; */
  /*     alpha = -INFINITY; */
  /*     for (int current_y = y; current_y >= 0; --current_y, current_x += dx) */
  /* 	{ */
  /* 	  int index = xy_to_index(cols, (int)current_x, current_y); */
  /* 	  float xchange = xres * (current_x - x); */
  /* 	  float ychange = yres * (current_y - y); */
  /* 	  float distance = sqrt(xchange*xchange + ychange*ychange); */
  /* 	  float elevation = src[index] - viewHeight; */
  /* 	  float angle = atan(elevation / distance); */

  /* 	  if (alpha <= angle) */
  /* 	    { */
  /* 	      alpha = angle; */
  /* 	      dst[index] = 1.0; */
  /* 	    } */
  /* 	} */

  /*     // south */
  /*     dx = ((float)(i - x))/(rows - y); */
  /*     current_x = x; */
  /*     alpha = -INFINITY; */
  /*     for (int current_y = y; current_y < rows; ++current_y, current_x += dx) */
  /* 	{ */
  /* 	  int index = xy_to_index(cols, (int)current_x, current_y); */
  /* 	  float xchange = xres * (current_x - x); */
  /* 	  float ychange = yres * (current_y - y); */
  /* 	  float distance = sqrt(xchange*xchange + ychange*ychange); */
  /* 	  float elevation = src[index] - viewHeight; */
  /* 	  float angle = atan(elevation / distance); */

  /* 	  if (alpha <= angle) */
  /* 	    { */
  /* 	      alpha = angle; */
  /* 	      dst[index] = 1.0; */
  /* 	    } */
  /* 	} */
  /*   } */
}
