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

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "index.h"

#define ENSURE(p, m) { if (!(p)) { fprintf(stderr, "Error: \"%s\" at %s:%d\n", m, __FILE__, __LINE__); exit(-1); } }


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

void * read_header(void * data,
                   const char * name_string, int version,
                   char ** projection_string,
                   double * x_min, double * x_max, double * y_min, double * y_max,
                   unsigned long long * sample_count) {

  uint64_t magic;
  int temp;

  // First half of magic number
  magic = *(uint64_t *)data;
  data += sizeof(magic);
  ENSURE(magic == MAGIC1, "MAGIC1");

  // Second half of magic number
  magic = *(uint64_t *)data;
  data += sizeof(magic);
  ENSURE(magic == MAGIC2, "MAGIC2");

  // Curve name
  temp = *(int *)data;
  data += sizeof(temp);
  ENSURE(strncmp(name_string, data, temp-1) == 0, "curve name");
  data += temp;

  // Curve version
  temp = *(int *)data;
  data += sizeof(temp);
  ENSURE(temp == version, "curve version");

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
  data += sizeof(temp);
  *sample_count = *(unsigned long long *)data;
  data += temp;

  return data;
}

void * map_index(const char * filename, struct stat * stat) {
  int fd = open(filename, O_RDONLY);
  void  * data;

  fstat(fd, stat);
  data = mmap(NULL, stat->st_size, PROT_READ, MAP_SHARED, fd, 0);
  close(fd);

  return data;
}

void unmap_index(void * data, const struct stat * stat) {
  munmap(data, stat->st_size);
}
