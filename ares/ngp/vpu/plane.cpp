auto VPU::renderPlane(n8 x, n8 y, Plane& plane) -> bool {
  x += plane.hscroll;
  y += plane.vscroll;

  u32 address = plane.address;
  address += (y >> 3) << 6;
  address += (x >> 3) << 1;

  x = (n3)x;
  y = (n3)y;

  auto& a = attributes[address >> 1];
  if(a.hflip == 0) x ^= 7;
  if(a.vflip == 1) y ^= 7;

  if(n2 index = characters[a.character][y][x]) {
    plane.priority = (&plane == &vpu.plane1) ^ vpu.io.planePriority;
    if(Model::NeoGeoPocket()) {
      if(index) plane.output = plane.palette[a.palette][index];
    }
    if(Model::NeoGeoPocketColor()) {
      if(dac.colorMode) {
        plane.output = colors[plane.colorCompatible + a.palette * 8 + plane.palette[a.palette][index]];
      } else {
        plane.output = colors[plane.colorNative + a.code * 4 + index];
      }
    }
    return true;
  }

  return false;
}
