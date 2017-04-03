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
#include <stdint.h>
#include <string.h>
#include "rasterio.h"


/*
 * Reference: http://www.gdal.org/gdal_tutorial.html
 */

void init()
{
  GDALAllRegister();
}

void load(const char * filename,
	  uint32_t * cols, uint32_t * rows,
	  double * transform,
	  char ** projection,
	  float ** image)
{
  GDALDatasetH dataset;
  GDALDriverH driver;
  GDALRasterBandH band;
  const char * proj;
  float * tmp;
  int origcols, origrows;

  dataset = GDALOpen(filename, GA_ReadOnly);
  if (!dataset)
    {
      fprintf(stderr, "Unable to open file %s %s:%d\n", filename, __FILE__, __LINE__);
      exit(-1);
    }
  driver = GDALGetDatasetDriver(dataset);
  band = GDALGetRasterBand(dataset, 1);
  if(GDALGetGeoTransform(dataset, transform) != CE_None)
    {
      fprintf(stderr, "Incomprehensible transform %s:%d\n", __FILE__, __LINE__);
      exit(-1);
    }
  proj = GDALGetProjectionRef(dataset);
  if (proj)
    {
      int n = strlen(proj);
      *projection = malloc(sizeof(char) * (n+1));
      strncpy(*projection, proj, n);
    }

  origcols = GDALGetRasterBandXSize(band);
  origrows = GDALGetRasterBandYSize(band);
  *cols = origcols % TILESIZE ? ((origcols >> TILEBITS) + 1) << TILEBITS : origcols;
  *rows = origrows % TILESIZE ? ((origrows >> TILEBITS) + 1) << TILEBITS : origrows;

  band = GDALGetRasterBand( dataset, 1 );

  if (!(tmp = (float *) CPLMalloc(sizeof(float) * *cols * *rows)))
    {
      fprintf(stderr, "CPLMalloc failed %s:%d\n", __FILE__, __LINE__);
      exit(-1);
    }

  if (!(*image = (float *) aligned_alloc(1<<12, sizeof(float) * *cols * *rows)))
    {
      fprintf(stderr, "aligned_alloc failed %s:%d\n", __FILE__, __LINE__);
      exit(-1);
    }

  if (GDALRasterIO(band, GF_Read, 0, 0, origcols, origrows, tmp, origcols, origrows, GDT_Float32, 0, sizeof(float) * *cols))
    {
      fprintf(stderr, "GDALRasterIO failed %s:%d\n", __FILE__, __LINE__);
      exit(-1);
    }

  memcpy(*image, tmp, sizeof(float) * *cols * *rows);
  CPLFree(tmp);
  GDALClose(dataset);
}

void dump(const char * filename,
	  uint32_t cols, uint32_t rows,
	  const double * transform,
	  const char * projection,
	  const float * image)
{
  GDALDatasetH dataset;
  GDALDriverH driver;
  GDALRasterBandH band;

  driver = GDALGetDriverByName("GTiff");
  if (!driver)
    {
      fprintf(stderr, "Unable to create \"GTiff\" driver %s:%d\n", __FILE__, __LINE__);
      exit(-1);
    }
  dataset = GDALCreate(driver, filename, cols, rows, 1, GDT_Float32, NULL);
  if (!dataset)
    {
      fprintf(stderr, "Unable to create dataset %s:%d\n", __FILE__, __LINE__);
      exit(-1);
    }
  GDALSetGeoTransform(dataset, transform);
  GDALSetProjection(dataset, projection);
  band = GDALGetRasterBand(dataset, 1);
  GDALRasterIO(band, GF_Write, 0, 0, cols, rows, image, cols, rows, GDT_Float32, 0, 0);
  GDALClose( dataset );
}

double xresolution(const double * transform)
{
  return transform[1] / 2.0;
}

double yresolution(const double * transform)
{
  return -transform[5] / 2.0;
}
