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
#include <dlfcn.h>

#include <pdal/Filter.hpp>
#include <pdal/io/LasHeader.hpp>
#include <pdal/io/LasReader.hpp>
#include <pdal/PointView.hpp>
#include <pdal/StageFactory.hpp>
#include <stxxl.h>

#include "pdal.h"


using namespace pdal;

typedef uint64_t(*to_curve)(double,double);
typedef void(*from_curve)(uint64_t,double *, double *);

point min_point;
point max_point;

typedef struct key_comparator {
  bool operator()(const point & a, const point & b) const {
    return (a.key < b.key);
  }

  const point & min_value() const {
    return min_point;
  }

  const point & max_value() const {
    return max_point;
  }

} key_comparator;


void pdal_load(const char * sofilename,
               const char ** filenamev,
               int filenamec,
               uint32_t cols, uint32_t rows,
               double * transform,
               char ** projection)
{
  void * handle;
  char * message;
  to_curve xy_to_curve;
  from_curve curve_to_xy;
  double x_min = std::numeric_limits<double>::max(), y_min = std::numeric_limits<double>::min();
  double x_max = std::numeric_limits<double>::max(), y_max = std::numeric_limits<double>::min();
  double x_range, y_range;

  /***********************************************************************
   * STXXL.  Reference: http://stxxl.org/tags/master/install_config.html *
   ***********************************************************************/
  stxxl::config * cfg = stxxl::config::get_instance();
  cfg->add_disk( stxxl::disk_config("disk=/tmp/StreetLevel.stxxl, 8 GiB, syscall unlink"));
  stxxl::VECTOR_GENERATOR<point>::result v;

  /********************
   * CALCULATE EXTENT *
   ********************/
  for (int i = 0; i < filenamec; ++i) {
    DimTypeList dims;
    LasHeader header;
    LasReader reader;
    Options options;
    PointTable table;
    PointViewSet set;

    options.add("filename", filenamev[i]);
    reader.setOptions(options);
    reader.prepare(table);
    header = reader.header();

    if (i == 0) {
      std::string proj = header.srs().getWKT().c_str();
      int n = proj.length();
      *projection = (char *)calloc(n+1, sizeof(char));
      strncpy(*projection, proj.c_str(), n);
    }

    if (x_min > header.minX())
      x_min = header.minX();
    if (y_min > header.minY())
      y_min = header.minY();
    if (x_max < header.maxX())
      x_max = header.maxX();
    if (y_max < header.maxY())
      y_max = header.maxY();

    set = reader.execute(table);
    fprintf(stderr, "dims = %ld\n", dims.size());
    fprintf(stderr, "pointCount = %ld\n", header.pointCount());
  }

  x_range = x_max - x_min;
  y_range = y_max - y_min;

  transform[0] = x_min; // top-left x
  transform[1] = x_range / cols; // west-east pixel resolution
  transform[2] = 0; // zero
  transform[3] = y_min; // top-left y
  transform[4] = 0; // zero
  transform[5] = y_range / rows; // north-south pixel resolution

  /**************
   * LOAD CURVE *
   **************/
  handle = dlopen(sofilename,  RTLD_NOW); // NULL implies failure
  if (handle == NULL) {
    fprintf(stderr, "%s\n", dlerror());
    exit(-1);
  }

  xy_to_curve = (to_curve)dlsym(handle, "xy_to_curve"); // NULL does not imply failure
  if ((message = dlerror()) != NULL) {
    fprintf(stderr, "%s\n", message);
    exit(-1);
  }

  curve_to_xy = (from_curve)dlsym(handle, "curve_to_xy"); // NULL does not imply failure
  if ((message = dlerror()) != NULL) {
    fprintf(stderr, "%s\n", message);
    exit(-1);
  }

  /***************
   * READ POINTS *
   ***************/
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

    for (uint j = 0; j < header.pointCount(); ++j) {
      point p;
      p.x = view->point(j).getFieldAs<double>(pdal::Dimension::Id::X);
      p.y = view->point(j).getFieldAs<double>(pdal::Dimension::Id::Y);
      p.z = view->point(j).getFieldAs<double>(pdal::Dimension::Id::Z);
      p.key = xy_to_curve((p.x - x_min)/x_range, (p.y - y_min)/y_range);
      v.push_back(p);
    }
  }

  /********
   * SORT *
   ********/
  min_point.key = 0;
  max_point.key = 0xffffffffffffffff;
  stxxl::sort(v.begin(), v.end(), key_comparator(), 1<<24);

  // SHOW A FEW POINTS
  for (unsigned int i = 0; i < 33; ++i) {
    double x, y;
    curve_to_xy(v[i].key, &x, &y);
    x = (x * x_range) + x_min;
    y = (y * y_range) + y_min;
    fprintf(stdout, "%016lx \t%.10lf \t%.10lf \t%.10lf \t%.10lf\n", v[i].key, v[i].x, x, v[i].y, y);
  }

}
