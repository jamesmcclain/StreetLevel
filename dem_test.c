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

#include "prefixsum.h"
#include "bitonic.h"
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
  /* char * projection; */
  /* double transform[6]; */
  int devices;
  opencl_struct info[4];
  struct timeval t1, t2, t3;

  if (argc < 3)
    {
      fprintf(stderr, "Not enough arguments %s:%d\n", __FILE__, __LINE__);
      exit(-1);
    }

  int order; sscanf(argv[2], "%d", &order);
  int n = 1<<(order);
  /* float * xs1 = malloc(sizeof(float) * n); */
  /* float * xs2 = malloc(sizeof(float) * n); */
  int * ys1 = malloc(sizeof(int) * n);
  int * ys2 = malloc(sizeof(int) * n);

  /**************
   * INITIALIZE *
   **************/
  opencl_init(4, &devices, info);
  srand((unsigned int)time(NULL));

  /***********
   * COMPUTE *
   ***********/
  /* pdal_load(argv[1], 1<<12, 1<<12, transform, &projection); */
  /* fprintf(stderr, "wkt = %s\n", projection); */
  /* fprintf(stderr, "%lf %lf %lf %lf %lf %lf\n", */
  /*         transform[0], transform[1], transform[2], */
  /*         transform[3], transform[4], transform[5]); */

  /* for (int i = 0; i < n; ++i) */
  /*   xs1[i] = xs2[i] = (float)rand()/(float)(RAND_MAX); */

  for (int i = 0; i < n; ++i)
    ys1[i] = ys2[i] = rand() % 17;

  /* gettimeofday(&t1, NULL); */
  /* bitonic(devices, info, xs1, n); */
  /* gettimeofday(&t2, NULL); */
  /* qsort(xs2, n, sizeof(float), compare); */
  /* gettimeofday(&t3, NULL); */

  prefixsum(0, info, ys1, n);
  for (int i = 1; i < n; ++i) ys2[i] += ys2[i-1];
  assert(memcmp(ys1, ys2, sizeof(float) * n) == 0);

  /* assert(memcmp(xs1, xs2, sizeof(float) * n) == 0); */
  /* fprintf(stdout, "bitonic: %8ld μs\n", (t2.tv_sec - t1.tv_sec) * 1000000 + (t2.tv_usec - t1.tv_usec)); */
  /* fprintf(stdout, "  qsort: %8ld μs\n", (t3.tv_sec - t2.tv_sec) * 1000000 + (t3.tv_usec - t2.tv_usec)); */

  /***********
   * CLEANUP *
   ***********/
  opencl_finit(devices, info);
  /* free(projection); */
  /* free(xs1); */
  /* free(xs2); */
  free(ys1);
  free(ys2);

  return 0;
}
