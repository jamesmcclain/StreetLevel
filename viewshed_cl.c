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
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <CL/cl.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "opencl.h"
#include "rasterio.h"


char * readfile(const char * filename)
{
  char * str;
  int fd, ret;
  struct stat buf;

  fd = open("./viewshed.cl", O_RDONLY);
  ENSURE(fstat(fd, &buf), ret);
  str = calloc(buf.st_size + 1, 1);
  ret = read(fd, str, buf.st_size);
  ENSURE(close(fd), ret);

  return str;
}

void viewshed_cl(int devices,
                 const opencl_struct * info,
                 const float * src, float * dst,
                 int cols, int rows,
                 int x, int y, int z,
                 double xres, double yres)
{
  const char * program_src;
  size_t program_src_length;
  size_t global_work_size[0];

  cl_event event;
  cl_int ret;
  cl_mem src_buffer, dst_buffer;
  cl_program program;
  cl_kernel kernel;

  // Create source, destination buffers
  // https://www.khronos.org/registry/OpenCL/sdk/1.2/docs/man/xhtml/clCreateBuffer.html
  src_buffer = clCreateBuffer(info[0].context,
                              CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                              sizeof(float) * cols * rows,
                              (void *)src,
                              &ret);
  ENSURE(ret, ret);
  dst_buffer = clCreateBuffer(info[0].context,
                              CL_MEM_WRITE_ONLY,
                              sizeof(float) * cols * rows,
                              NULL,
                              &ret);
  ENSURE(ret, ret);

  // Load, build program
  program_src = readfile("./viewshed.cl");
  program_src_length = strlen(program_src);
  // https://www.khronos.org/registry/OpenCL/sdk/1.2/docs/man/xhtml/clCreateProgramWithSource.html
  program = clCreateProgramWithSource(info[0].context, 1, &program_src, &program_src_length, &ret);
  ENSURE(ret, ret);
  free((void *)program_src);
  // https://www.khronos.org/registry/OpenCL/sdk/1.2/docs/man/xhtml/clBuildProgram.html
  ENSURE(clBuildProgram(program, 1, &(info[0].device), NULL, NULL, NULL), ret);

  // Setup kernel
  // https://www.khronos.org/registry/OpenCL/sdk/1.2/docs/man/xhtml/clCreateKernel.html
  kernel = clCreateKernel(program, "viewshed", &ret);
  ENSURE(ret, ret);
  // https://www.khronos.org/registry/OpenCL/sdk/1.2/docs/man/xhtml/clSetKernelArg.html
  ENSURE(clSetKernelArg(kernel, 0, sizeof(cl_mem), &src_buffer), ret);
  ENSURE(clSetKernelArg(kernel, 1, sizeof(cl_mem), &dst_buffer), ret);

  // Enqueue kernel
  // https://www.khronos.org/registry/OpenCL/sdk/1.2/docs/man/xhtml/clEnqueueNDRangeKernel.html
  global_work_size[0] = 8192;
  ENSURE(clEnqueueNDRangeKernel(info[0].queue, kernel, 1,
                                NULL, global_work_size, NULL,
                                0, NULL, &event), ret);

  // Read result from device
  // https://www.khronos.org/registry/OpenCL/sdk/1.2/docs/man/xhtml/clEnqueueReadBuffer.html
  ENSURE(clEnqueueReadBuffer(info[0].queue,
                             dst_buffer,
                             CL_TRUE,
                             0, sizeof(float) * cols * rows, dst,
                             1, &event, NULL), ret);

  /* ret = clFlush(info[0].queue); */
  /* ret = clFinish(info[0].queue); */
  /* ret = clReleaseKernel(kernel); */
  /* ret = clReleaseProgram(program); */
  /* ret = clReleaseMemObject(src_buffer); */
  /* ret = clReleaseMemObject(dst_buffer); */
  /* ret = clReleaseCommandQueue(info[0].queue); */
  /* ret = clReleaseContext(info[0].context); */
}
