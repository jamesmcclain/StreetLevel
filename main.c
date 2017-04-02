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
#include <inttypes.h>
#include <time.h>
#include "rasterio.h"
#include "viewshed.h"


int main(int argc, char ** argv)
{
  float * src, * dst;
  uint64_t cols, rows;
  double transform[6];
  char * projection;
  time_t before, after;

  if (argc < 3)
    {
      fprintf(stderr, "Not enough arguments %s:%d\n", __FILE__, __LINE__);
      exit(-1);
    }

  init();
  load(argv[1], &cols, &rows, transform, &projection, &src);
  dst = calloc(cols * rows, sizeof(float));
  before = time(NULL);
  viewshed(src, dst, cols, rows, xresolution(transform), yresolution(transform));
  after = time(NULL);
  dump(argv[2], cols, rows, transform, projection, dst);
  fprintf(stdout, "%d seconds\n", after - before);

  free(dst);

  return 0;
}
