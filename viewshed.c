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
#include <math.h>


void viewshed(const float * src, float * dst,
	      uint64_t cols, uint64_t rows,
	      double xres, double yres)
{
  int x = 4608;
  int y = 3072;
  float viewHeight = 2000;

  for (int i = 0; i < rows; ++i)
    {
      float dy, currenty, alpha;

      // west
      dy = ((float)(i - y))/(cols - x);
      currenty = y;
      alpha = -INFINITY;
      for (int currentx = x; currentx >= 0; --currentx, currenty += dy)
	{
	  float xchange = xres * (currentx - x);
	  float ychange = yres * (currenty - y);
	  float distance = sqrt(xchange*xchange + ychange*ychange);
	  float elevation = src[(int)currenty * cols + currentx] - viewHeight;
	  float angle = atan(elevation / distance);

	  if (alpha <= angle)
	    {
	      alpha = angle;
	      dst[(int)currenty * cols + currentx] = 1.0;
	    }
	}
    }
  
  for (int i = 0; i < cols; ++i)
    {
      float dx, currentx, alpha;
      
      // north
      dx = ((float)(i - x))/y;
      currentx = x;
      alpha = -INFINITY;
      for (int currenty = y; currenty >= 0; --currenty, currentx += dx)
	{
	  float xchange = xres * (currentx - x);
	  float ychange = yres * (currenty - y);
	  float distance = sqrt(xchange*xchange + ychange*ychange);
	  float elevation = src[currenty * cols + (int)currentx] - viewHeight;
	  float angle = atan(elevation / distance);

	  if (alpha <= angle)
	    {
	      alpha = angle;
	      dst[currenty * cols + (int)currentx] = 1.0;
	    }
	}

      // south
      dx = ((float)(i - x))/(rows - y);
      currentx = x;
      alpha = -INFINITY;
      for (int currenty = y; currenty < rows; ++currenty, currentx += dx)
	{
	  float xchange = xres * (currentx - x);
	  float ychange = yres * (currenty - y);
	  float distance = sqrt(xchange*xchange + ychange*ychange);
	  float elevation = src[currenty * cols + (int)currentx] - viewHeight;
	  float angle = atan(elevation / distance);

	  if (alpha <= angle)
	    {
	      alpha = angle;
	      dst[currenty * cols + (int)currentx] = 1.0;
	    }
	}
    }
}
