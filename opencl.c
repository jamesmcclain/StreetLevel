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
#include <fcntl.h>
#include <unistd.h>
#include <CL/cl.h>
#include <sys/stat.h>
#include "opencl.h"

char * readfile(const char * filename)
{
  char * str;
  int fd, ret;
  struct stat buf;

  fd = open(filename, O_RDONLY);
  ENSURE(fstat(fd, &buf), ret);
  str = calloc(buf.st_size + 1, 1);
  ret = read(fd, str, buf.st_size);
  ENSURE(close(fd), ret);

  return str;
}

void opencl_init(int N, int * n, opencl_struct * info)
{
  cl_int ret;
  cl_platform_id platforms[4];
  cl_uint num_platforms;

  *n = 0;

  // Query Platforms
  ENSURE(clGetPlatformIDs(0, NULL, &num_platforms), ret);
  ENSURE(clGetPlatformIDs(SMALLER(num_platforms, N), platforms, &num_platforms), ret);

  for (int i = 0; i < num_platforms; ++i)
    {
      cl_device_id devices[4];
      cl_uint num_devices;

      // Query Devices
      ret = clGetDeviceIDs(platforms[i], CL_DEVICE_TYPE_GPU, 0, NULL, &num_devices);
      ret |= clGetDeviceIDs(platforms[i], CL_DEVICE_TYPE_GPU, SMALLER(num_devices, N), devices, &num_devices);
      if (ret) num_devices = 0;

      // Contexts and Command Queues
      for (int j = 0; j < num_devices; ++j, ++(*n))
        {
          info[*n].platform = platforms[i];
          info[*n].device = devices[j];
          info[*n].context = clCreateContext(NULL, 1, &devices[j], NULL, NULL, &ret);
          ENSURE(ret, ret);
          info[*n].queue = clCreateCommandQueue(info[*n].context, info[*n].device, 0, &ret);
          ENSURE(ret, ret);
        }
    }
}

void opencl_finit(int n, opencl_struct * info)
{
  cl_int ret;

  for (int i = 0; i < n; ++i)
    {
      // https://www.khronos.org/registry/OpenCL/sdk/1.2/docs/man/xhtml/clFinish.html
      ENSURE(clFinish(info[i].queue), ret);
      // https://www.khronos.org/registry/OpenCL/sdk/1.2/docs/man/xhtml/clReleaseCommandQueue.html
      ENSURE(clReleaseCommandQueue(info[i].queue), ret);
      // https://www.khronos.org/registry/OpenCL/sdk/1.2/docs/man/xhtml/clReleaseContext.html
      ENSURE(clReleaseContext(info[i].context), ret);
    }
}
