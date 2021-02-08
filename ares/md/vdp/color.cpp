auto VDP::color(n32 color) -> n64 {
  n32 R = color.bit(0, 2);
  n32 G = color.bit(3, 5);
  n32 B = color.bit(6, 8);
  n32 M = color.bit(9,10);

  u32 lookup[3][8] = {
    {  0,  29,  52,  70,  87, 101, 116, 130},  //shadow
    {  0,  52,  87, 116, 144, 172, 206, 255},  //normal
    {130, 144, 158, 172, 187, 206, 228, 255},  //highlight
  };

  n64 r = image::normalize(lookup[M][R], 8, 16);
  n64 g = image::normalize(lookup[M][G], 8, 16);
  n64 b = image::normalize(lookup[M][B], 8, 16);

  return r << 32 | g << 16 | b << 0;
}
