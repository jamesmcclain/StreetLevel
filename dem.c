#include <stdio.h>
#include <stdlib.h>

#include "curve/curve_interface.h"
#include "index/index.h"


int main(int argc, const char ** argv)
{
  void * data;
  char * projection;
  double x_min, x_max, y_min, y_max;
  unsigned long long sample_count;
  struct stat stat;

  if (argc < 3) {
    fprintf(stderr, "Usage: %s <so_filename> <index_filename>\n", argv[0]);
    exit(-1);
  }

  load_curve(argv[1]);
  data = map_index(argv[2], &stat);

  data = read_header(data,
                     curve_name(), curve_version(),
                     &projection,
                     &x_min, &x_max, &y_min, &y_max,
                     &sample_count);

  fprintf(stdout, "projection = %s \n", projection);
  fprintf(stdout, "bounding box = %lf %lf %lf %lf\n", x_min, x_max, y_min, y_max);
  fprintf(stderr, "sample count = %lld\n", sample_count);

  unmap_index(data, &stat);
}
