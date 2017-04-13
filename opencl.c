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
#include <math.h>
#include <CL/cl.h>
#include "opencl.h"


#define N (8)

#define MIN(a,b) (a < b ? a: b)
#define ENSURE(call, r) { if (r = (call)) { fprintf(stderr, "Non-zero return code %d %s:%d\n", r, __FILE__, __LINE__); exit(-1); } }

typedef struct {
  cl_platform_id platform;
  cl_device_id device;
  uint8_t gpu;
} device_struct;


void opencl_init()
{
  cl_int ret;
  cl_platform_id platforms[N];
  cl_uint num_platforms;

  device_struct ds[N];
  int dsn = 0;

  // Query Platforms
  ENSURE(clGetPlatformIDs(0, NULL, &num_platforms), ret);
  ENSURE(clGetPlatformIDs(MIN(num_platforms, N), platforms, &num_platforms), ret);

  for (int i = 0; i < num_platforms; ++i)
    {
      cl_device_id devices[N];
      cl_uint num_devices;

      // Query Devices
      ENSURE(clGetDeviceIDs(platforms[i], CL_DEVICE_TYPE_ALL, 0, NULL, &num_devices), ret);
      ENSURE(clGetDeviceIDs(platforms[i], CL_DEVICE_TYPE_ALL, MIN(num_devices, N), devices, &num_devices), ret);
      for (int j = 0; j < num_devices; ++j)
        {
          ds[dsn].platform = platforms[i];
          ds[dsn++].device = devices[j];
        }
    }

  for (int i = 0; i < dsn; ++i)
    {
      fprintf(stdout, "platform = %d, device = %d\n", ds[i].platform, ds[i].device);
    }
}
