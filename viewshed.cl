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
                       __global float * dst)
{
  int cols = 9216;
  int rows = 6144;
  float xres = 305.748113;
  float yres = 305.748113;

  int row = get_global_id(0);

  int x = 4608;
  int y = 3072;
  float viewHeight = 2000.0;
  float dy = ((float)(row - y)) / (cols - x);
  float dm = sqrt(xres*xres + dy*dy*yres*yres);
  float alpha = -INFINITY;
  float current_y = convert_float(y);
  float current_distance = 0.0;

  if (row < rows)
    {
      for (int col = x; col < cols; ++col, current_y += dy, current_distance += dm)
        {
          int index = xy_to_fancy_index(cols, col, convert_int(current_y));
          float elevation = src[index] - viewHeight;
          float angle = elevation / current_distance;

          if (alpha < angle)
            {
              index = xy_to_vanilla_index(cols, col, convert_int(current_y));
              alpha = angle;
              dst[index] = 1.0;
            }
        }
    }
}
