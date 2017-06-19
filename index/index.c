#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include "index.h"

unsigned long long write_header(int fd,
                                const char * name_string, int version,
                                const char * projection_string,
                                double x_min, double x_max, double y_min, double y_max,
                                unsigned long long sample_count) {
  uint64_t magic1 = MAGIC1;
  uint64_t magic2 = MAGIC2;
  unsigned long long bytes = 0;
  int temp;

  bytes += write(fd, &magic1, sizeof(magic1));
  bytes += write(fd, &magic2, sizeof(magic2));
  temp = strlen(name_string) + 1;
  bytes += write(fd, &temp, sizeof(temp));
  bytes += write(fd, name_string, temp);
  bytes += write(fd, &version, sizeof(version));
  temp = strlen(projection_string) + 1;
  bytes += write(fd, &temp, sizeof(temp));
  bytes += write(fd, projection_string, temp);
  bytes += write(fd, &x_min, sizeof(x_min));
  bytes += write(fd, &x_max, sizeof(x_max));
  bytes += write(fd, &y_min, sizeof(y_min));
  bytes += write(fd, &y_max, sizeof(y_max));
  temp = sizeof(sample_count);
  bytes += write(fd, &temp, sizeof(temp));
  bytes += write(fd, &sample_count, sizeof(sample_count));

  return bytes;
}

const void * read_header(const void * data,
                         const char * name_string, int version,
                         char ** projection_string,
                         double * x_min, double * x_max, double * y_min, double * y_max,
                         unsigned long long * sample_count) {

  uint64_t magic;
  int temp;

  // First half of magic number
  magic = *(uint64_t *)data;
  data += sizeof(magic);
  if (magic != MAGIC1) exit(-1);

  // Second half of magic number
  magic = *(uint64_t *)data;
  data += sizeof(magic);
  if (magic != MAGIC2) exit(-1);

  // Curve name
  temp = *(int *)data;
  data += sizeof(temp);
  if (!strncmp(name_string, data, temp-1)) exit(-1);
  data += temp;

  // Curve version
  temp = *(int *)data;
  data += sizeof(temp);
  if (temp != version) exit(-1);

  // Projection
  temp = *(int *)data;
  data += sizeof(temp);
  *projection_string = calloc(temp, 1);
  strncpy(*projection_string, data, temp-1);
  data += temp;

  // Bounding box
  *x_min = *(double *)data;
  data += sizeof(double);
  *x_max = *(double *)data;
  data += sizeof(double);
  *y_min = *(double *)data;
  data += sizeof(double);
  *y_max = *(double *)data;
  data += sizeof(double);

  // Sample count
  temp = *(int *)data;
  *sample_count = *(unsigned long long *)data;
  data += temp;

  return data;
}
