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

typedef float floatv __attribute__ ((vector_size (sizeof(float) * REGISTERSIZE))) __attribute__ ((aligned));
typedef int intv __attribute__ ((vector_size (sizeof(int) * REGISTERSIZE))) __attribute__ ((aligned));


void viewshed(const float * src, float * dst,
	      uint32_t cols, uint32_t rows,
	      double xres, double yres)
{
  float * alphas = NULL;
  float * dys = NULL;
  float * dms = NULL;
  int larger = (cols > rows ? cols : rows);
  float multipliers[TILESIZE];

  int x = 4608; // XXX
  int y = 3072; // XXX
  float viewHeight = 2000; // XXX

  ALLOC(alphas, larger);
  ALLOC(dys, larger);
  ALLOC(dms, larger);

  for (int k = 0; k < TILESIZE; ++k) multipliers[k] = (float)k;

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
      // TILESIZE, but may be something else on the first iteration in
      // order to get aligned with tiles/pages.
      if (i == x) { for (; (i + width) % TILESIZE; ++width); } else { width = TILESIZE; }

      for (int j = 0; j < rows; ++j) // for each ray (indexed by final row)
	{
	  float __attribute__ ((aligned)) dy = dys[j];
	  int __attribute__ ((aligned)) last_y = LASTY(dy);

	  // Minimize repeatedly-evaluation of the same pixel by
	  // skipping overlapping rays.  If the last y value of this
	  // ray-chunk is the same as that of the next ray-chunk, then
	  // defer to the latter.  Otherwise, if they are different,
	  // evaluate this chunk of the ray.
	  if (j == rows-1 || last_y != LASTY(dys[j+1]))
	    {
	      // restore context from arrays
	      float __attribute__ ((aligned)) dm = dms[j];
	      float __attribute__ ((aligned)) alpha = alphas[j];
	      float __attribute__ ((aligned)) current_y = y + (i - x) * dy;
	      float __attribute__ ((aligned)) current_distance = (i - x) * dm;

#ifdef __AVX__
	      // Extend ray to the East.  In the common case, use
	      // vectors ...
	      if (width == TILESIZE)
		{
		  floatv mults;
		  floatv ys;
		  floatv distance;
		  floatv elevation;
		  floatv angle;

		  for (int k = 0; k < TILESIZE; k+= REGISTERSIZE)
		    {
		      mults = __builtin_ia32_loadups256(multipliers + k);
		      ys = __builtin_ia32_addps256(__builtin_ia32_mulps256(mults, __builtin_ia32_vbroadcastss256(&dy)), __builtin_ia32_vbroadcastss256(&current_y));
		      for (int l = 0; l < REGISTERSIZE; ++l)
			{
			  int fancy_index = xy_to_fancy_index(cols, (i + k + l), (int)ys[l]);
			  elevation[l] = src[fancy_index] - viewHeight;
			}
		      distance = __builtin_ia32_addps256(__builtin_ia32_mulps256(mults, __builtin_ia32_vbroadcastss256(&dm)), __builtin_ia32_vbroadcastss256(&current_distance));
		      angle = __builtin_ia32_divps256(elevation, distance);

		      for (int l = 0; l < REGISTERSIZE; ++l)
		    	{
		    	  if (alpha < angle[l])
		    	    {
			      int index = xy_to_vanilla_index(cols, (i + k + l), (int)ys[l]);
		    	      alpha = angle[l];
		    	      dst[index] = 1.0;
		    	    }
		    	}
		    }
		}
	      // ... in the special case, use scalers.
	      else
#endif
		{
		  for (int k = 0; k < width; ++k, current_y += dy, current_distance += dm)
		    {
		      int current_x = i + k;
		      int fancy_index = xy_to_fancy_index(cols, current_x, (int)current_y);
		      float elevation = src[fancy_index] - viewHeight;
		      float angle = elevation / current_distance;

		      if (alpha < angle)
			{
			  int index = xy_to_vanilla_index(cols, (i + k), (int)current_y);
			  alpha = angle;
			  dst[index] = 1.0;
			}
		    }
		}
	      
	      // save context for this ray and all that overlap it
	      for (int k = j; last_y == LASTY(dys[k]); --k) alphas[k] = alpha;
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
