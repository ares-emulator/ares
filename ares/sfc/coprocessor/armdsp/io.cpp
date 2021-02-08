//MMIO: 00-3f,80-bf:3800-38ff
//3800-3807 mirrored throughout
//a0 ignored

auto ARMDSP::read(n24 address, n8) -> n8 {
  cpu.synchronize(*this);

  n8 data = 0x00;
  address &= 0xff06;

  if(address == 0x3800) {
    if(bridge.armtocpu.ready) {
      bridge.armtocpu.ready = false;
      data = bridge.armtocpu.data;
    }
  }

  if(address == 0x3802) {
    bridge.signal = false;
  }

  if(address == 0x3804) {
    data = bridge.status();
  }

  return data;
}

auto ARMDSP::write(n24 address, n8 data) -> void {
  cpu.synchronize(*this);

  address &= 0xff06;

  if(address == 0x3802) {
    bridge.cputoarm.ready = true;
    bridge.cputoarm.data = data;
  }

  if(address == 0x3804) {
    data &= 1;
    if(!bridge.reset && data) reset();
    bridge.reset = data;
  }
}
