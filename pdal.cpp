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

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <pdal/Filter.hpp>
#include <pdal/io/LasHeader.hpp>
#include <pdal/io/LasReader.hpp>
#include <pdal/PointView.hpp>
#include <pdal/StageFactory.hpp>
#include "pdal.h"


using namespace pdal;

void pdal_load(const char * filename,
               uint32_t cols, uint32_t rows,
               double * transform,
               char ** projection)
{
  DimTypeList dims;
  LasHeader header;
  LasReader reader;
  Options options;
  PointTable table;
  PointViewPtr view;
  PointViewSet set;
  size_t point_size;

  options.add("filename", filename);
  reader.setOptions(options);
  reader.prepare(table);
  header = reader.header();

  std::string proj = header.srs().getWKT().c_str();
  int n = proj.length();
  *projection = (char *)calloc(n+1, sizeof(char));
  strncpy(*projection, proj.c_str(), n);

  transform[0] = header.minX(); // top-left x
  transform[1] = (header.maxX() - header.minX()) / cols; // west-east pixel resolution
  transform[2] = 0; // zero
  transform[3] = header.maxY(); // top-left y
  transform[4] = 0; // zero
  transform[5] = (header.minY() - header.maxY()) / rows; // north-south pixel resolution

  set = reader.execute(table);
  view = *(set.begin());
  dims = view->dimTypes();
  point_size = view->pointSize();
  // // fprintf(stderr, "pts = %d dims = %d point_size = %d\n", view->size(), dims, point_size);
  // PointRef point_ref1 = view->point(0);
  // PointRef point_ref2 = view->point(1);
  fprintf(stderr, "dims = %d\n", dims.size());
  for (int i = 0; i < dims.size(); ++i)
    {
      fprintf(stderr, "\t%d = %s %s\n", i,
              pdal::Dimension::interpretationName(dims.at(i).m_type).c_str(),
              pdal::Dimension::description(dims.at(i).m_id).c_str());
    }
}
