auto PPU::color(n32 color) -> n64 {
  n32 R = color.bit( 0, 4);
  n32 G = color.bit( 5, 9);
  n32 B = color.bit(10,14);

  n64 r = image::normalize(R, 5, 16);
  n64 g = image::normalize(G, 5, 16);
  n64 b = image::normalize(B, 5, 16);

  if(colorEmulation->value()) {
    f64 lcdGamma = 4.0, outGamma = 2.2;
    f64 lb = pow(B / 31.0, lcdGamma);
    f64 lg = pow(G / 31.0, lcdGamma);
    f64 lr = pow(R / 31.0, lcdGamma);
    r = pow((  0 * lb +  50 * lg + 255 * lr) / 255, 1 / outGamma) * (0xffff * 255 / 280);
    g = pow(( 30 * lb + 230 * lg +  10 * lr) / 255, 1 / outGamma) * (0xffff * 255 / 280);
    b = pow((220 * lb +  10 * lg +  50 * lr) / 255, 1 / outGamma) * (0xffff * 255 / 280);
  }

  return r << 32 | g << 16 | b << 0;
}
