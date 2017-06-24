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

#ifndef __INDEX_H__
#define __INDEX_H__

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define MAGIC1 (0x466c6f72656e6365)
#define MAGIC2 (0x4e6f726d6e646965)

typedef struct query_key {
  uint64_t key;
  unsigned long long * index;
} query_key;

#ifdef __cplusplus
extern "C" {
#endif

  unsigned long long write_header(int fd,
                                  const char * name_string, int version,
                                  const char * projection_string,
                                  double x_min, double x_max, double y_min, double y_max,
                                  unsigned long long sample_count);

  void * read_header(void * data,
                     const char * name_string, int version,
                     char ** projection_string,
                     double * x_min, double * x_max, double * y_min, double * y_max,
                     unsigned long long * sample_count);

  void * map_index(const char * filename, struct stat * stat);

  void unmap_index(void * data, const struct stat * stat);

#ifdef __cplusplus
}
#endif

#endif
