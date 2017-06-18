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

#include <sys/time.h>

#include <pdal/Filter.hpp>
#include <pdal/io/LasHeader.hpp>
#include <pdal/io/LasReader.hpp>
#include <pdal/PointView.hpp>
#include <pdal/StageFactory.hpp>
#include <stxxl/sorter>

#include "curve/curve_interface.h"
#include "pdal.h"

using namespace pdal;

// Sorting
pdal_point min_point;
pdal_point max_point;

typedef struct key_comparator {
  bool operator()(const pdal_point & a, const pdal_point & b) const {
    return (a.key < b.key);
  }

  const pdal_point & min_value() const {
    return min_point;
  }

  const pdal_point & max_value() const {
    return max_point;
  }

} key_comparator;


typedef stxxl::sorter<pdal_point, key_comparator> point_sorter;


void pdal_load(const char ** filenamev,
               int filenamec,
               uint32_t cols, uint32_t rows) {
  double x_min = std::numeric_limits<double>::max(), y_min = std::numeric_limits<double>::max();
  double x_max = std::numeric_limits<double>::min(), y_max = std::numeric_limits<double>::min();
  double x_range, y_range;
  char * projection = NULL;

  struct timeval t1, t2, t3, t4, t5, t6;

  /***********************************************************************
   * STXXL.  Reference: http://stxxl.org/tags/master/install_config.html *
   ***********************************************************************/
  stxxl::config * cfg = stxxl::config::get_instance();
  cfg->add_disk( stxxl::disk_config("disk=/tmp/StreetLevel.stxxl, 0, syscall unlink"));
  min_point.key = 0;
  max_point.key = 0xffffffffffffffff;
  point_sorter sorter(key_comparator(), (1<<30));

  /************************
   * COMPUTE BOUNDING BOX *
   ************************/
  gettimeofday(&t1, NULL);
  for (int i = 0; i < filenamec; ++i) {
    DimTypeList dims;
    LasHeader header;
    LasReader reader;
    Options options;
    PointTable table;
    PointViewSet set;
    PointViewPtr view;

    options.add("filename", filenamev[i]);
    reader.setOptions(options);
    reader.prepare(table);
    header = reader.header();

    double h_x_min = header.minX();
    double h_x_max = header.maxX();
    double h_y_min = header.minY();
    double h_y_max = header.maxY();

    if (h_x_min < x_min) x_min = h_x_min;
    if (h_x_max > x_max) x_max = h_x_max;
    if (h_y_min < y_min) y_min = h_y_min;
    if (h_y_max > y_max) y_max = h_y_max;
  }
  x_range = x_max - x_min;
  y_range = y_max - y_min;
  gettimeofday(&t2, NULL);
  fprintf(stdout, "bounding box: %ld μs\n", (t2.tv_sec - t1.tv_sec) * 1000000 + (t2.tv_usec - t1.tv_usec));

  /***************
   * READ POINTS *
   ***************/
  gettimeofday(&t3, NULL);
  for (int i = 0; i < filenamec; ++i) {
    DimTypeList dims;
    LasHeader header;
    LasReader reader;
    Options options;
    PointTable table;
    PointViewSet set;
    PointViewPtr view;

    options.add("filename", filenamev[i]);
    reader.setOptions(options);
    reader.prepare(table);
    header = reader.header();
    set = reader.execute(table);
    view = *(set.begin());

    if (i == 0) {
      std::string proj = header.srs().getWKT().c_str();
      int n = proj.length();
      projection = (char *)calloc(n+1, sizeof(char));
      strncpy(projection, proj.c_str(), n);
    }

    for (uint j = 0; j < header.pointCount(); ++j) {
      pdal_point p;
      p.x = view->point(j).getFieldAs<double>(pdal::Dimension::Id::X);
      p.y = view->point(j).getFieldAs<double>(pdal::Dimension::Id::Y);
      p.z = view->point(j).getFieldAs<double>(pdal::Dimension::Id::Z);
      p.key = xy_to_curve((p.x - x_min)/x_range, (p.y - y_min)/y_range);
      sorter.push(p);
    }
  }
  gettimeofday(&t4, NULL);
  fprintf(stdout, "input: %ld μs\n", (t4.tv_sec - t2.tv_sec) * 1000000 + (t4.tv_usec - t3.tv_usec));

  fprintf(stdout, "x: %.10lf \t%.10lf\n", x_min, x_max);
  fprintf(stdout, "y: %.10lf \t%.10lf\n", y_min, y_max);
  fprintf(stdout, "samples: %lld\n", sorter.size());

  /********
   * SORT *
   ********/
  gettimeofday(&t5, NULL);
  sorter.sort();
  gettimeofday(&t6, NULL);
  fprintf(stdout, "sort: %ld μs\n", (t6.tv_sec - t5.tv_sec) * 1000000 + (t6.tv_usec - t5.tv_usec));

  /*********************
   * SHOW A FEW POINTS *
   *********************/
  for (unsigned int i = 0; i < 33; ++i, ++sorter) {
    double x, y;
    pdal_point p = *sorter;

    curve_to_xy(p.key, &x, &y);
    x = (x * x_range) + x_min;
    y = (y * y_range) + y_min;
    fprintf(stdout, "%016lx \t%.10lf \t%.10lf \t%.10lf \t%.10lf\n", p.key, p.x, x, p.y, y);
  }

  free(projection);
}
