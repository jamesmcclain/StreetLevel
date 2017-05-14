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
#include <string.h>
#include "opencl.h"
#include "partition.h"

void partition(int device, const opencl_struct * info, float * _xs, cl_float pivot, size_t _n)
{
  const char * program_src;
  size_t program_src_length;

  cl_int ret, n = _n;
  cl_mem xs, xs_prime, bitsums;
  cl_program program;
  cl_kernel sum, filter, part;

  int max_k = 0;
  for (max_k = 31; (max_k >= 0) && !(_n & (1<<max_k)); --max_k); // n assumed to be a power of two

  /******************
   * CREATE BUFFERS *
   ******************/
  // https://www.khronos.org/registry/OpenCL/sdk/1.2/docs/man/xhtml/clCreateBuffer.html
  xs = clCreateBuffer(info[device].context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, sizeof(cl_float) * _n, (void *)_xs, &ret); ENSURE(ret, ret);
  xs_prime = clCreateBuffer(info[device].context, CL_MEM_READ_WRITE, sizeof(cl_float) * _n, NULL, &ret); ENSURE(ret, ret);
  bitsums  = clCreateBuffer(info[device].context, CL_MEM_READ_WRITE, sizeof(cl_int)   * _n, NULL, &ret); ENSURE(ret, ret);

  /***********************
   * LOAD, BUILD PROGRAM *
   ***********************/
  program_src = readfile("./sort.cl");
  program_src_length = strlen(program_src);
  // https://www.khronos.org/registry/OpenCL/sdk/1.2/docs/man/xhtml/clCreateProgramWithSource.html
  program = clCreateProgramWithSource(info[device].context, 1, &program_src, &program_src_length, &ret); ENSURE(ret, ret);
  free((void *)program_src);
  // https://www.khronos.org/registry/OpenCL/sdk/1.2/docs/man/xhtml/clBuildProgram.html
  ENSURE(clBuildProgram(program, 1, &(info[device].device), NULL, NULL, NULL), ret);

  /*****************
   * SETUP KERNELS *
   *****************/
  // https://www.khronos.org/registry/OpenCL/sdk/1.2/docs/man/xhtml/clCreateKernel.html
  sum = clCreateKernel(program, "sum", &ret); ENSURE(ret, ret);
  filter = clCreateKernel(program, "filter", &ret); ENSURE(ret, ret);
  part = clCreateKernel(program, "partition", &ret); ENSURE(ret, ret);

  /*******************
   * EXECUTE KERNELS *
   *******************/
  // FILTER
  // https://www.khronos.org/registry/OpenCL/sdk/1.2/docs/man/xhtml/clSetKernelArg.html
  ENSURE(clSetKernelArg(filter, 0, sizeof(cl_mem), &xs), ret);
  ENSURE(clSetKernelArg(filter, 1, sizeof(cl_mem), &bitsums), ret);
  ENSURE(clSetKernelArg(filter, 2, sizeof(cl_float), &pivot), ret);
  {
    size_t last = n>>1;
    ENSURE(clEnqueueNDRangeKernel(info[device].queue, filter, 1, NULL, &last, NULL, 0, NULL, NULL), ret);
  }

  // PREFIX SUM
  ENSURE(clSetKernelArg(sum, 0, sizeof(cl_mem), &bitsums), ret);
  ENSURE(clSetKernelArg(sum, 1, sizeof(cl_int), &n), ret);
  for (int k = 1; k < max_k; ++k)
    {
      cl_int start = ((1<<k)-1);
      cl_int log_stride = k+1;
      size_t last = ((_n - start)>>log_stride) + 1; // Want ids from 0 to ((_n - start)>>log_stride) inclusive
      cl_int length = 1<<k;

      ENSURE(clSetKernelArg(sum, 2, sizeof(cl_int), &start), ret);
      ENSURE(clSetKernelArg(sum, 3, sizeof(cl_int), &log_stride), ret);
      ENSURE(clSetKernelArg(sum, 4, sizeof(cl_int), &length), ret);
      // https://www.khronos.org/registry/OpenCL/sdk/1.2/docs/man/xhtml/clEnqueueNDRangeKernel.html
      ENSURE(clEnqueueNDRangeKernel(info[device].queue, sum, 1, NULL, &last, NULL, 0, NULL, NULL), ret);
    }
  for (int k = max_k-1; k >= 0; --k)
    {
      cl_int start = ((1<<(k+1))-1);
      cl_int log_stride = k+1;
      size_t last = ((_n - start)>>log_stride) + 1;
      cl_int length = 1<<k;

      ENSURE(clSetKernelArg(sum, 2, sizeof(cl_int), &start), ret);
      ENSURE(clSetKernelArg(sum, 3, sizeof(cl_int), &log_stride), ret);
      ENSURE(clSetKernelArg(sum, 4, sizeof(cl_int), &length), ret);
      ENSURE(clEnqueueNDRangeKernel(info[device].queue, sum, 1,
                                    NULL, &last, NULL,
                                    0, NULL, NULL), ret);
    }

  // PARTITION w/ PREFIX SUM
  ENSURE(clSetKernelArg(part, 0, sizeof(cl_mem), &bitsums), ret);
  ENSURE(clSetKernelArg(part, 1, sizeof(cl_mem), &xs), ret);
  ENSURE(clSetKernelArg(part, 2, sizeof(cl_mem), &xs_prime), ret);
  ENSURE(clSetKernelArg(part, 3, sizeof(cl_int), &n), ret);
  ENSURE(clEnqueueNDRangeKernel(info[device].queue, part, 1, NULL, &_n, NULL, 0, NULL, NULL), ret);

  /***************************
   * READ RESULT FROM DEVICE *
   ***************************/
  // https://www.khronos.org/registry/OpenCL/sdk/1.2/docs/man/xhtml/clEnqueueReadBuffer.html
  ENSURE(clEnqueueReadBuffer(info[device].queue, xs_prime, CL_TRUE, 0, sizeof(cl_float) * _n, _xs, 0, NULL, NULL), ret);

  /*********************
   * RELEASE RESOURCES *
   *********************/
  // https://www.khronos.org/registry/OpenCL/sdk/1.2/docs/man/xhtml/clReleaseKernel.html
  ENSURE(clReleaseKernel(sum), ret);
  ENSURE(clReleaseKernel(filter), ret);
  // https://www.khronos.org/registry/OpenCL/sdk/1.2/docs/man/xhtml/clReleaseProgram.html
  ENSURE(clReleaseProgram(program), ret);
  // https://www.khronos.org/registry/OpenCL/sdk/1.2/docs/man/xhtml/clReleaseMemObject.html
  ENSURE(clReleaseMemObject(xs), ret);
  ENSURE(clReleaseMemObject(xs_prime), ret);
  ENSURE(clReleaseMemObject(bitsums), ret);
}
