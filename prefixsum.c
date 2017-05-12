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


// Reference: https://en.wikipedia.org/wiki/Prefix_sum#Parallel_algorithm

void sum(int * xs, int n, int start, int log_stride, int length, int i)
{
  i = (i<<log_stride) + start;
  if (i + length < n)
    xs[i + length] += xs[i];
}

void prefixsum(int * xs, int n)
{
  int max_k = 0;
  for (max_k = 31; (max_k >= 0) && !(n & (1<<max_k)); --max_k); // n assumed to be a power of two

  for (int k = 0; k < max_k; ++k)
    {
      int start = ((1<<k)-1);
      int log_stride = k+1;
      int last = (n - start)>>log_stride;
      int length = 1<<k;

      for (int i = 0; i <= last; ++i)
        sum(xs, n, start, log_stride, length, i);
    }

  for (int k = max_k-1; k >= 0; --k)
    {
      int start = ((1<<(k+1))-1);
      int log_stride = k+1;
      int last = (n - start)>>log_stride;
      int length = 1<<k;

      for (int i = 0; i <= last; ++i)
        sum(xs, n, start, log_stride, length, i);
    }
}
