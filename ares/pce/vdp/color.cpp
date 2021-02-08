auto VDP::color(n32 color) -> n64 {
  n3 B = color.bit(0,2);
  n3 R = color.bit(3,5);
  n3 G = color.bit(6,8);
  n1 M = color.bit(9);

  n64 r = image::normalize(R, 3, 16);
  n64 g = image::normalize(G, 3, 16);
  n64 b = image::normalize(B, 3, 16);

  if(M == 0) {
    //color
    return r << 32 | g << 16 | b << 0;
  } else {
    //grayscale
    n64 l = r * 0.2126 + g * 0.7152 + b * 0.0722;
    return l << 32 | l << 16 | l << 0;
  }
}
