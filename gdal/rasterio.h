/*
 * Copyright (c) 2017, James McClain
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
 *    includes software developed by Dr. James W. McClain.
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

#ifndef __RASTERIO_H__
#define __RASTERIO_H__

#include <stdint.h>
#include "gdal.h"
#include "cpl_conv.h"
#include "ogr_api.h"
#include "ogr_srs_api.h"


#define PAGEBITS (12)
#define PAGESIZE (1<<PAGEBITS)

#define TILEBITS (5)
#define TILESIZE (1<<TILEBITS)
#define TILEMASK (0x1f)

#define SUBTILEBITS (2)
#define SUBTILESIZE (1<<SUBTILEBITS)
#define SUBTILEMASK (0x03)

#define REGISTERSIZE (8)

extern void rasterio_init();
extern void rasterio_load(const char * filename,
                          uint32_t * cols, uint32_t * rows,
                          double * transform,
                          char ** projection,
                          float ** image);
extern void rasterio_dump(const char * filename,
                          uint32_t cols, uint32_t rows,
                          double * transform,
                          const char * projection,
                          float * image);
extern double x_resolution(const double * transform);
extern double y_resolution(const double * transform);
extern uint32_t xy_to_fancy_index(uint32_t cols, uint32_t x, uint32_t y);
extern uint32_t xy_to_vanilla_index(uint32_t cols, uint32_t x, uint32_t y);

#endif
