#include <stdint.h>
#include <string.h>
#include <unistd.h>
/* #include <sys/time.h> */
/* #include <sys/types.h> */
/* #include <sys/stat.h> */
/* #include <fcntl.h> */

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
  bytes += write(fd, &sample_count, sizeof(sample_count));

  return bytes;
}
