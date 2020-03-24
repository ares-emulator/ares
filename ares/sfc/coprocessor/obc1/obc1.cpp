OBC1 obc1;
#include "serialization.cpp"

auto OBC1::unload() -> void {
  ram.reset();
}

auto OBC1::power() -> void {
  status.baseptr = (ramRead(0x1ff5) & 1) ? 0x1800 : 0x1c00;
  status.address = (ramRead(0x1ff6) & 0x7f);
  status.shift   = (ramRead(0x1ff6) & 3) << 1;
}

auto OBC1::read(uint24 address, uint8) -> uint8 {
  address &= 0x1fff;

  switch(address) {
  case 0x1ff0: return ramRead(status.baseptr + (status.address << 2) + 0);
  case 0x1ff1: return ramRead(status.baseptr + (status.address << 2) + 1);
  case 0x1ff2: return ramRead(status.baseptr + (status.address << 2) + 2);
  case 0x1ff3: return ramRead(status.baseptr + (status.address << 2) + 3);
  case 0x1ff4: return ramRead(status.baseptr + (status.address >> 2) + 0x200);
  }

  return ramRead(address);
}

auto OBC1::write(uint24 address, uint8 data) -> void {
  address &= 0x1fff;

  switch(address) {
  case 0x1ff0: ramWrite(status.baseptr + (status.address << 2) + 0, data); return;
  case 0x1ff1: ramWrite(status.baseptr + (status.address << 2) + 1, data); return;
  case 0x1ff2: ramWrite(status.baseptr + (status.address << 2) + 2, data); return;
  case 0x1ff3: ramWrite(status.baseptr + (status.address << 2) + 3, data); return;
  case 0x1ff4: {
    uint8 temp = ramRead(status.baseptr + (status.address >> 2) + 0x200);
    temp = (temp & ~(3 << status.shift)) | ((data & 3) << status.shift);
    ramWrite(status.baseptr + (status.address >> 2) + 0x200, temp);
  } return;
  case 0x1ff5:
    status.baseptr = (data & 1) ? 0x1800 : 0x1c00;
    ramWrite(address, data);
    return;
  case 0x1ff6:
    status.address = (data & 0x7f);
    status.shift   = (data & 3) << 1;
    ramWrite(address, data);
    return;
  case 0x1ff7:
    ramWrite(address, data);
    return;
  }

  return ramWrite(address, data);
}

auto OBC1::ramRead(uint address) -> uint8 {
  return ram.read(address & 0x1fff);
}

auto OBC1::ramWrite(uint address, uint8 data) -> void {
  ram.write(address & 0x1fff, data);
}
