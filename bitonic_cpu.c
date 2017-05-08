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

// Reference: https://en.wikipedia.org/wiki/Bitonic_sorter

void level(float * xs, int n, int l, int k)
{
  for (int i = 0; (i + (1<<k)) < n; ++i)
    {
      int a = i;
      int b = i + (1<<k);

      int a1 = a % (1<<(l+2));
      int b1 = b % (1<<(l+2));
      int cut1 = (1<<(l+1));

      int a2 = a % (1<<(k+2));
      int b2 = b % (1<<(k+2));
      int cut2 = (1<<(k+1));

      if (a1 < cut1 && b1 < cut1) // down arrows
        {
          if ((a2 < cut2 && b2 < cut2) || (a2 >= cut2 && b2 >= cut2))
            {
              float _a = xs[a], _b = xs[b];
              /* fprintf(stdout, "down: %f %f\n", _a, _b); */
              if (_a > _b) xs[a] = _b, xs[b] = _a;
            }
        }
      else if (a1 >= cut1 && b1 >= cut1) // up arrows
        {
          if ((a2 < cut2 && b2 < cut2) || (a2 >= cut2 && b2 >= cut2))
            {
              float _a = xs[a], _b = xs[b];
              /* fprintf(stdout, "  up: %f %f\n", _a, _b); */
              if (_a < _b) xs[a] = _b, xs[b] = _a;
            }
        }
    }
}

void bitonic_cpu(float * xs, int n)
{
  int l = 0;
  for (l = 31; (l >= 0) && !(n & (1<<l)); --l) // n assumed to be a power of two

  for (int i = 0; i < l; ++i)
    for (int j = i; j >= 0; --j)
      level(xs, n, i, j);
}