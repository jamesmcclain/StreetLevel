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

// Reference: https://en.wikipedia.org/wiki/Bitonic_sorter

void down_arrow(float * xs, int n)
{
  float a = xs[0], b = xs[1<<n];
  if (b < a) xs[0] = b, xs[1<<n] = a;
}

void up_arrow(float * xs, int n)
{
  float a = xs[0], b = xs[1<<n];
  if (b > a) xs[0] = b, xs[1<<n] = a;
}

void dark_red(float * xs, int n)
{
  for (int i = 0; i < (1<<n); ++i)
    down_arrow(xs+i, n);
}

void light_red(float * xs, int n)
{
  for (int i = 0; i < (1<<n); ++i)
    up_arrow(xs+i, n);
}

void blue(float * xs, int n)
{
  dark_red(xs, n);
  if (n > 0)
    {
      blue(xs, n-1);
      blue(xs+(1<<n), n-1);
    }
}

void green(float * xs, int n)
{
  light_red(xs, n);
  if (n > 0)
    {
      green(xs, n-1);
      green(xs+(1<<n), n-1);
    }
}

void bitonic_cpu(float * xs, int n)
{
  for (int i = 0; i < n; ++i)
    {
      for (int j = 0; j < (1<<(n+1)); j+=(1<<(i+2)))
        {
          blue(xs+j, i);
          green(xs+(1<<(i+1))+j, i);
        }
    }

  blue(xs, n);
}
