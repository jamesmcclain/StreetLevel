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

#include "opencl.h"
#include "gdal/rasterio.h"


void viewshed(int device,
              const opencl_struct * info,
              const float * src, float * dst,
              int cols, int rows,
              int x, int y, float z,
              double xres, double yres) {
  const char * program_src;
  size_t program_src_length;

  cl_int ret;
  cl_mem src_buffer, dst_buffer, alphas;
  cl_program program;
  cl_kernel kernel;

  cl_float _xres, _yres;
  cl_float _z = z;
  cl_int _cols, _rows, _x, _y;
  cl_int flip, transpose;
  cl_int this_steps, that_steps;
  size_t global_work_size = 0;

  /******************
   * CREATE BUFFERS *
   ******************/
  // https://www.khronos.org/registry/OpenCL/sdk/1.2/docs/man/xhtml/clCreateBuffer.html
  src_buffer = clCreateBuffer(info[device].context,
                              CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                              sizeof(float) * cols * rows,
                              (void *)src,
                              &ret);
  ENSURE(ret, ret);
  dst_buffer = clCreateBuffer(info[device].context,
                              CL_MEM_WRITE_ONLY | CL_MEM_COPY_HOST_PTR,
                              sizeof(float) * cols * rows,
                              (void *)dst,
                              &ret);
  ENSURE(ret, ret);
  alphas = clCreateBuffer(info[device].context,
                          CL_MEM_READ_WRITE,
                          sizeof(float) * LARGER(cols, rows),
                          NULL,
                          &ret);
  ENSURE(ret, ret);

  /***********************
   * LOAD, BUILD PROGRAM *
   ***********************/
  program_src = readfile("./viewshed.cl");
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
  kernel = clCreateKernel(program, "viewshed", &ret);
  ENSURE(ret, ret);

  /******************
   * EXECUTE KERNEL *
   ******************/
  // https://www.khronos.org/registry/OpenCL/sdk/1.2/docs/man/xhtml/clSetKernelArg.html
  ENSURE(clSetKernelArg(kernel, 0, sizeof(cl_mem), &src_buffer), ret);
  ENSURE(clSetKernelArg(kernel, 1, sizeof(cl_mem), &dst_buffer), ret);
  ENSURE(clSetKernelArg(kernel, 2, sizeof(cl_mem), &alphas), ret);
  ENSURE(clSetKernelArg(kernel, 7, sizeof(cl_float), &_z), ret);
  for (int cardinal = 0; cardinal < 4; ++cardinal) {
    global_work_size = 0;
    this_steps = that_steps = -1;

    if (cardinal == 0) { // East
      flip = 0, transpose = 0;
      _x = x, _y = y, _cols = cols, _rows = rows;
      _xres = xres, _yres = yres;
    }
    else if (cardinal == 1) { // South
      flip = 0, transpose = 1;
      _x = (_y-1), _y = x, _cols = rows, _rows = cols;
      _xres = yres, _yres = xres;
    }
    else if (cardinal == 2) { // West
      flip = 1, transpose = 0;
      _x = cols-x, _y = y, _cols = cols, _rows = rows;
      _xres = xres, _yres = yres;
    }
    else if (cardinal == 3) { // North
      flip = 1, transpose = 1;
      _x = rows-y, _y = x, _cols = rows, _rows = cols;
      _xres = yres, _yres = xres;
    }

    ENSURE(clSetKernelArg(kernel, 3, sizeof(cl_int), &_cols), ret);
    ENSURE(clSetKernelArg(kernel, 4, sizeof(cl_int), &_rows), ret);
    ENSURE(clSetKernelArg(kernel, 5, sizeof(cl_int), &_x), ret);
    ENSURE(clSetKernelArg(kernel, 6, sizeof(cl_int), &_y), ret);
    ENSURE(clSetKernelArg(kernel, 8, sizeof(cl_float), &_xres), ret);
    ENSURE(clSetKernelArg(kernel, 9, sizeof(cl_float), &_yres), ret);
    ENSURE(clSetKernelArg(kernel, 10, sizeof(cl_int), &flip), ret);
    ENSURE(clSetKernelArg(kernel, 11, sizeof(cl_int), &transpose), ret);

    //Enqueue kernel the correct number of times per column of tiles
    for (cl_int start_col = _x, width = 1; start_col < _cols; start_col += width) {
      if (start_col == _x) for (; (start_col + width) % TILESIZE; ++width);
      else width = TILESIZE;

      cl_int stop_col = SMALLER(start_col + width, _cols);
      that_steps = this_steps;
      this_steps = (int)(((float)(_cols-_x))/(stop_col-_x));
      global_work_size = (_rows / this_steps) + 1;

      ENSURE(clSetKernelArg(kernel, 12, sizeof(cl_int), &start_col), ret);
      ENSURE(clSetKernelArg(kernel, 13, sizeof(cl_int), &stop_col), ret);
      ENSURE(clSetKernelArg(kernel, 14, sizeof(cl_int), &this_steps), ret);
      ENSURE(clSetKernelArg(kernel, 15, sizeof(cl_int), &that_steps), ret);

      // https://www.khronos.org/registry/OpenCL/sdk/1.2/docs/man/xhtml/clEnqueueNDRangeKernel.html
      ENSURE(clEnqueueNDRangeKernel(info[device].queue, kernel, 1,
                                    NULL, &global_work_size, NULL,
                                    0, NULL, NULL), ret);
    }
  }

  /***************************
   * READ RESULT FROM DEVICE *
   ***************************/
  // https://www.khronos.org/registry/OpenCL/sdk/1.2/docs/man/xhtml/clEnqueueReadBuffer.html
  ENSURE(clEnqueueReadBuffer(info[device].queue,
                             dst_buffer,
                             CL_TRUE,
                             0, sizeof(float) * cols * rows, dst,
                             0, NULL, NULL), ret);

  /*********************
   * RELEASE RESOURCES *
   *********************/
  // https://www.khronos.org/registry/OpenCL/sdk/1.2/docs/man/xhtml/clReleaseKernel.html
  ENSURE(clReleaseKernel(kernel), ret);
  // https://www.khronos.org/registry/OpenCL/sdk/1.2/docs/man/xhtml/clReleaseProgram.html
  ENSURE(clReleaseProgram(program), ret);
  // https://www.khronos.org/registry/OpenCL/sdk/1.2/docs/man/xhtml/clReleaseMemObject.html
  ENSURE(clReleaseMemObject(src_buffer), ret);
  ENSURE(clReleaseMemObject(dst_buffer), ret);
  ENSURE(clReleaseMemObject(alphas), ret);
}
