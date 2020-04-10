auto VPU::renderPlane(uint8 x, uint8 y, Plane& plane) -> bool {
  x += plane.hscroll;
  y += plane.vscroll;

  uint address = plane.address;
  address += (y >> 3) << 6;
  address += (x >> 3) << 1;

  x = (uint3)x;
  y = (uint3)y;

  auto& a = attributes[address >> 1];
  if(a.hflip == 0) x ^= 7;
  if(a.vflip == 1) y ^= 7;

  if(uint2 index = characters[a.character][y][x]) {
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
