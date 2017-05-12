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
#include "prefixsum.h"

void prefixsum(int device, const opencl_struct * info, int * xs, cl_int n)
{
  const char * program_src;
  size_t program_src_length;

  cl_int ret;
  cl_mem buffer;
  cl_program program;
  cl_kernel kernel;

  int max_k = 0;
  for (max_k = 31; (max_k >= 0) && !(n & (1<<max_k)); --max_k); // n assumed to be a power of two

  /*****************
   * CREATE BUFFER *
   *****************/
  // https://www.khronos.org/registry/OpenCL/sdk/1.2/docs/man/xhtml/clCreateBuffer.html
  buffer = clCreateBuffer(info[device].context,
                          CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR,
                          sizeof(cl_int) * n,
                          (void *)xs,
                          &ret);
  ENSURE(ret, ret);

  /***********************
   * LOAD, BUILD PROGRAM *
   ***********************/
  program_src = readfile("./sort.cl");
  program_src_length = strlen(program_src);
  // https://www.khronos.org/registry/OpenCL/sdk/1.2/docs/man/xhtml/clCreateProgramWithSource.html
  program = clCreateProgramWithSource(info[device].context, 1, &program_src, &program_src_length, &ret);
  ENSURE(ret, ret);
  free((void *)program_src);
  // https://www.khronos.org/registry/OpenCL/sdk/1.2/docs/man/xhtml/clBuildProgram.html
  ENSURE(clBuildProgram(program, 1, &(info[device].device), NULL, NULL, NULL), ret);

  /****************
   * SETUP KERNEL *
   ****************/
  // https://www.khronos.org/registry/OpenCL/sdk/1.2/docs/man/xhtml/clCreateKernel.html
  kernel = clCreateKernel(program, "sum", &ret);
  ENSURE(ret, ret);

  /******************
   * EXECUTE KERNEL *
   ******************/
  // https://www.khronos.org/registry/OpenCL/sdk/1.2/docs/man/xhtml/clSetKernelArg.html
  ENSURE(clSetKernelArg(kernel, 0, sizeof(cl_mem), &buffer), ret);
  ENSURE(clSetKernelArg(kernel, 1, sizeof(cl_int), &n), ret);
  for (int k = 0; k < max_k; ++k)
    {
      cl_int start = ((1<<k)-1);
      cl_int log_stride = k+1;
      size_t last = ((n - start)>>log_stride) + 1; // Want ids from 0 to ((n - start)>>log_stride) inclusive
      cl_int length = 1<<k;

      ENSURE(clSetKernelArg(kernel, 2, sizeof(cl_int), &start), ret);
      ENSURE(clSetKernelArg(kernel, 3, sizeof(cl_int), &log_stride), ret);
      ENSURE(clSetKernelArg(kernel, 4, sizeof(cl_int), &length), ret);
      // https://www.khronos.org/registry/OpenCL/sdk/1.2/docs/man/xhtml/clEnqueueNDRangeKernel.html
      ENSURE(clEnqueueNDRangeKernel(info[device].queue, kernel, 1,
                                    NULL, &last, NULL,
                                    0, NULL, NULL), ret);
    }
  for (int k = max_k-1; k >= 0; --k)
    {
      cl_int start = ((1<<(k+1))-1);
      cl_int log_stride = k+1;
      size_t last = ((n - start)>>log_stride) + 1;
      cl_int length = 1<<k;

      ENSURE(clSetKernelArg(kernel, 2, sizeof(cl_int), &start), ret);
      ENSURE(clSetKernelArg(kernel, 3, sizeof(cl_int), &log_stride), ret);
      ENSURE(clSetKernelArg(kernel, 4, sizeof(cl_int), &length), ret);
      ENSURE(clEnqueueNDRangeKernel(info[device].queue, kernel, 1,
                                    NULL, &last, NULL,
                                    0, NULL, NULL), ret);
    }

  /***************************
   * READ RESULT FROM DEVICE *
   ***************************/
  // https://www.khronos.org/registry/OpenCL/sdk/1.2/docs/man/xhtml/clEnqueueReadBuffer.html
  ENSURE(clEnqueueReadBuffer(info[device].queue,
                             buffer,
                             CL_TRUE,
                             0, sizeof(cl_int) * n, xs,
                             0, NULL, NULL), ret);

  /*********************
   * RELEASE RESOURCES *
   *********************/
  // https://www.khronos.org/registry/OpenCL/sdk/1.2/docs/man/xhtml/clReleaseKernel.html
  ENSURE(clReleaseKernel(kernel), ret);
  // https://www.khronos.org/registry/OpenCL/sdk/1.2/docs/man/xhtml/clReleaseProgram.html
  ENSURE(clReleaseProgram(program), ret);
  // https://www.khronos.org/registry/OpenCL/sdk/1.2/docs/man/xhtml/clReleaseMemObject.html
  ENSURE(clReleaseMemObject(buffer), ret);
}
