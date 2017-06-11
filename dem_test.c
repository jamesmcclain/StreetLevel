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
#include <assert.h>
#include <string.h>
#include <time.h>

#include <sys/time.h>

#include "opencl.h"
#include "pdal.h"


int compare(const void * _a, const void * _b)
{
  float a = *(float *)_a;
  float b = *(float *)_b;

  if (a < b) return -1;
  else if (a > b) return 1;
  else return 0;
}

int main(int argc, char ** argv)
{
  char * projection;
  double transform[6];
  int devices;
  opencl_struct info[4];

  if (argc < 3)
    {
      fprintf(stderr, "Not enough arguments %s:%d\n", __FILE__, __LINE__);
      exit(-1);
    }

  /**************
   * INITIALIZE *
   **************/
  opencl_init(4, &devices, info);

  /***********
   * COMPUTE *
   ***********/
  pdal_load(argv[1], argv[2], 1<<12, 1<<12, transform, &projection);

  /**********
   * OUTPUT *
   **********/
  fprintf(stderr, "wkt = %s\n", projection);
  fprintf(stderr, "transform = %lf %lf %lf %lf %lf %lf\n",
          transform[0], transform[1], transform[2],
          transform[3], transform[4], transform[5]);

  /***********
   * CLEANUP *
   ***********/
  opencl_finit(devices, info);
  free(projection);

  return 0;
}
