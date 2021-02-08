auto APU::Channel5::run() -> void {
  i11 output;
  switch(r.scale) {
  case 0: output = (n8)s.data << 3 - r.volume; break;
  case 1: output = (n8)s.data - 0x100 << 3 - r.volume; break;
  case 2: output = (i8)s.data << 3 - r.volume; break;
  case 3: output = (n8)s.data << 3; break;
  }

  o.left  = r.leftEnable  ? output : (i11)0;
  o.right = r.rightEnable ? output : (i11)0;
}
