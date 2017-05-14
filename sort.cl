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


// Reference: https://en.wikipedia.org/wiki/Prefix_sum#Parallel_algorithm
__kernel void sum(__global int * bitsum, int n, int start, int log_stride, int length)
{
  int i = (get_global_id(0)<<log_stride) + start;
  int j = i + length;

  if (j < n)
    bitsum[j] += bitsum[i];
}

// Reference: https://courses.cs.washington.edu/courses/cse332/10sp/lectures/lecture20.pdf
__kernel void filter(__global float * xs, __global int * bitsum, float pivot)
{
  int i = (get_global_id(0)<<1);
  int j = i + 1;
  int i_bit = xs[i] < pivot ? 1:0;
  int j_bit = xs[j] < pivot ? 1:0;

  bitsum[i] = i_bit;
  bitsum[j] = i_bit + j_bit;
}

__kernel void partition(__global int * bitsums, __global float * src, __global float * dst, int n)
{
  int i = get_global_id(0);

  if (i == 0)
    {
      if (bitsums[0] == 1)
        dst[0] = src[0]; // below pivot
      else
        dst[n-1] = src[0]; // above pivot
    }
  else if (i > 0)
    {
      if (bitsums[i] > bitsums[i-1])
        dst[bitsums[i]-1] = src[i]; // below pivot
      else
        dst[n-1 - (i - bitsums[i])] = src[i]; // above pivot
    }
}

// Reference: https://en.wikipedia.org/wiki/Bitonic_sorter
__kernel void bitonic(__global float * xs, int level, int k)
{
  int i = get_global_id(0);

  int a = i;
  int b = i + (1<<k);

  int a1 = a % (1<<(level+2));
  int b1 = b % (1<<(level+2));
  int cut1 = (1<<(level+1));

  int a2 = a % (1<<(k+2));
  int b2 = b % (1<<(k+2));
  int cut2 = (1<<(k+1));

  if (a1 < cut1 && b1 < cut1) // down arrows
    {
      if ((a2 < cut2 && b2 < cut2) || (a2 >= cut2 && b2 >= cut2))
        {
          float _a = xs[a], _b = xs[b];
          if (_a > _b) xs[a] = _b, xs[b] = _a;
        }
    }
  else if (a1 >= cut1 && b1 >= cut1) // up arrows
    {
      if ((a2 < cut2 && b2 < cut2) || (a2 >= cut2 && b2 >= cut2))
        {
          float _a = xs[a], _b = xs[b];
          if (_a < _b) xs[a] = _b, xs[b] = _a;
        }
    }
}
