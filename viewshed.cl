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

#define TILEBITS (5)
#define TILESIZE (1<<TILEBITS)
#define TILEMASK (0x1f)

#define SUBTILEBITS (2)
#define SUBTILESIZE (1<<SUBTILEBITS)
#define SUBTILEMASK (0x03)

int xy_to_fancy_index(int cols, int x, int y)
{
  int tile_row_words = cols * TILESIZE; // words per row of tiles
  int tile_x = x >> TILEBITS; // column of tile that (x,y) is in
  int tile_y = y >> TILEBITS; // row of tile that (x,y) is in
  int major_index = (tile_y * tile_row_words) + (tile_x * TILESIZE * TILESIZE); // index of tile

  int subtile_row_words = TILESIZE * SUBTILESIZE; // words per row of subtiles
  int subtile_x = (x & TILEMASK) >> SUBTILEBITS; // column of subtile (within tile) that (x,y) is in
  int subtile_y = (y & TILEMASK) >> SUBTILEBITS; // row of subtile that (x,y) is in
  int minor_index = (subtile_y * subtile_row_words) + (subtile_x * SUBTILESIZE * SUBTILESIZE); // index of subtile

  int micro_row_words = SUBTILESIZE; // words per row within a subtile
  int micro_x = x & SUBTILEMASK; // column within subtile
  int micro_y = y & SUBTILEMASK; // row within subtile
  int micro_index = (micro_y * micro_row_words) + micro_x;

  return major_index + minor_index + micro_index;
}

int xy_to_vanilla_index(int cols, int x, int y)
{
  return (y*cols) + x;
}

__kernel void viewshed(__global float * src,
                       __global float * dst,
                       __global float * alphas,
                       int cols, int rows,
                       int x, int y, float viewHeight,
                       float xres, float yres,
                       int flip, int transpose,
                       int start_col, int stop_col,
                       int this_steps, int that_steps)
{
  int gid = get_global_id(0);
  int row = gid * this_steps;

  if (!(row < rows) && (row - this_steps < rows))
    row = rows-1;

  // If this ray-chunk does not overlap others too much, then compute it.
  if (row < rows)
    {
      float dy = ((float)(row - y)) / (cols - x);
      float dm = sqrt(xres*xres + dy*dy*yres*yres);
      float current_y = y + ((start_col - x)*dy);
      float current_distance = (1/MAXFLOAT) + ((start_col - x)*dm);
      float alpha;

      if (start_col == x) alpha = -INFINITY;
      else alpha = alphas[row];

      for (int col = start_col; col < stop_col; ++col, current_y += dy, current_distance += dm)
        {
          int index;
          if (!flip && !transpose) index = xy_to_fancy_index(cols, col, convert_int(current_y));
          else if (flip && !transpose) index = xy_to_fancy_index(cols, (cols-1-col), convert_int(current_y));
          else if (!flip && transpose) index = xy_to_fancy_index(rows, convert_int(current_y), col);
          else if (flip && transpose) index = xy_to_fancy_index(rows, convert_int(current_y), (cols-1-col));
          float curve = 6378137 * (1 - cos(current_distance / 6378137));
          float elevation = src[index] - viewHeight - curve;
          float angle = elevation / current_distance;

          if (alpha < angle)
            {
              if (!flip && !transpose) index = xy_to_vanilla_index(cols, col, convert_int(current_y));
              else if (flip && !transpose) index = xy_to_vanilla_index(cols, (cols-col-1), convert_int(current_y));
              else if (!flip && transpose) index = xy_to_vanilla_index(rows, convert_int(current_y), col);
              else if (flip && transpose) index = xy_to_vanilla_index(rows, convert_int(current_y), (cols-col-1));
              alpha = angle;
              dst[index] = 1.0;
            }
        }

      for (int i = row; (i < row + this_steps) && (i < rows); ++i)
        alphas[i] = alpha;
    }
}
