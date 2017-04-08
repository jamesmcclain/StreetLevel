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
  const char * proj = NULL;
  float * tmp = NULL;
  uint32_t orig_cols, orig_rows;

  dataset = GDALOpen(filename, GA_ReadOnly);  // GDAL dataset
  if (!dataset)
    {
      fprintf(stderr, "Unable to open file %s %s:%d\n", filename, __FILE__, __LINE__);
      exit(-1);
    }
  driver = GDALGetDatasetDriver(dataset); // GDAL driver
  band = GDALGetRasterBand(dataset, 1); // Get the first band
  if (GDALGetGeoTransform(dataset, transform) != CE_None) // Get the transform
    {
      fprintf(stderr, "Incomprehensible transform %s:%d\n", __FILE__, __LINE__);
      exit(-1);
    }
  if ((proj = GDALGetProjectionRef(dataset))) // Get the projection
    {
      int n = strlen(proj);
      *projection = malloc(sizeof(char) * (n+1));
      strncpy(*projection, proj, n);
    }

  orig_cols = GDALGetRasterBandXSize(band); // Number of columns
  orig_rows = GDALGetRasterBandYSize(band); // Number of rows
  *cols = orig_cols % TILESIZE ? (((orig_cols >> TILEBITS) + 1) << TILEBITS) : orig_cols; // Padded columns
  *rows = orig_rows % TILESIZE ? (((orig_rows >> TILEBITS) + 1) << TILEBITS) : orig_rows; // Padded rows

  if (!(tmp = (float *) CPLMalloc(sizeof(float) * *cols * *rows))) // Untiled image
    {
      fprintf(stderr, "CPLMalloc failed %s:%d\n", __FILE__, __LINE__);
      exit(-1);
    }

  if (!(*image = (float *) aligned_alloc(PAGESIZE, sizeof(float) * *cols * *rows))) // Tiled image
    {
      fprintf(stderr, "aligned_alloc failed %s:%d\n", __FILE__, __LINE__);
      exit(-1);
    }

  if (GDALRasterIO(band, GF_Read, // Copy data
		   0, 0,
		   orig_cols, orig_rows,
		   tmp,
		   orig_cols, orig_rows,
		   GDT_Float32,
		   0, sizeof(float) * *cols))
    {
      fprintf(stderr, "GDALRasterIO failed %s:%d\n", __FILE__, __LINE__);
      exit(-1);
    }

  for (int i = 0; i < *cols; ++i) // Copy untiled image data into tiled array
    {
      for (int j = 0; j < *rows; ++j)
	{
	  int src_index = j * *cols + i;
	  int dst_index = xy_to_fancy_index(*cols, i, j);
	  (*image)[dst_index] = tmp[src_index];
	}
    }

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
  if (GDALRasterIO(band, GF_Write,
		   0, 0,
		   cols, rows,
		   image,
		   cols, rows,
		   GDT_Float32,
		   0, 0))
    {
      fprintf(stderr, "GDALRasterIO failed %s:%d\n", __FILE__, __LINE__);
      exit(-1);
    }

  GDALClose(dataset);
}

double x_resolution(const double * transform)
{
  return transform[1] / 2.0;
}

double y_resolution(const double * transform)
{
  return -transform[5] / 2.0;
}

uint32_t xy_to_fancy_index(uint32_t cols, uint32_t x, uint32_t y)
{
  int tile_row_words = cols * TILESIZE; // words per row of tiles
  int tile_x = x >> TILEBITS; // column of tile that (x,y) is in
  int tile_y = y >> TILEBITS; // row of tile that (x,y) is in
  int major_index = (tile_y * tile_row_words) + (tile_x * TILESIZE * TILESIZE); // index of tile

  int subtile_row_words = TILESIZE * SUBTILESIZE; // words per row of subtiles
  int subtile_x = (x & TILEMASK) >> SUBTILEBITS; // column of subtile (within tile) that (x,y) is in
  int subtile_y = (y & TILEMASK) >> SUBTILEBITS; // row of subtile that (x,y) is in
  int minor_index = (subtile_y * subtile_row_words) + (subtile_x * SUBTILESIZE * SUBTILESIZE); // index of subtile

  int micro_row_words = SUBTILESIZE; // words per row within a subtile
  int micro_x = x & SUBTILEMASK; // column within subtile
  int micro_y = y & SUBTILEMASK; // row within subtile
  int micro_index = (micro_y * micro_row_words) + micro_x;

  return major_index + minor_index + micro_index;
}

uint32_t xy_to_vanilla_index(uint32_t cols, uint32_t x, uint32_t y)
{
  return (y*cols) + x;
}
