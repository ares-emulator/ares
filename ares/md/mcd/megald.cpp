auto MCD::LD::load() -> void {
  driveState = 0x02; // 0x02 = CD door closed
}

auto MCD::LD::unload() -> void {
}

auto MCD::LD::read(n24 address) -> n8 {
  bool isOutput = (address & 0x80);
  u8 regNum = (address & 0x3f) >> 1;

  n8 data = isOutput ? output[regNum] : input[regNum];

  switch(address) {
    case 0xfdfe81: break; // output[0]
    case 0xfdfe85:
       data.bit(7) = 0; // 1 = LD in tray
       data.bit(5) = 1; // 1 = CD in tray
       data.bit(4) = 0; // 1 = CD door closed and empty
       break;
    case 0xfdfe8b:
      data = 0x7f; // No buttons pressed (remote)
      break;
    case 0xfdfe8d:
      data = driveState;
      break;
    default:
      debug(unimplemented, "[MCD::readLD] address=0x", hex(address, 8L), " reg=0x", hex(regNum, 2L));
      break;
  }

  return data;
}
    
auto MCD::LD::write(n24 address, n8 data) -> void {
  bool isOutput = (address & 0x80);
  u8 regNum = (address & 0x3f) >> 1;

  if(isOutput) {
    output[regNum] = data;
  } else {
    input[regNum] = data;
  }

  switch(address) {
    case 0xfdfe41:
      if(data.bit(7)) output[0] = data;
      break;
    case 0xfdfe45:
      // TODO: U7, U5, U4
      if(data.bit(0, 3) != 0) driveState = data.bit(0, 3);
      break;
    default:
      debug(unimplemented, "[MCD::writeLD] address=0x", hex(address, 8L), " reg=0x", hex(regNum, 2L), " value=0x", hex(data, 4L));
      break;
  }
}

