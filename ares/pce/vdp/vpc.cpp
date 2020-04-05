inline auto VPC::bus(uint16 hcounter) const -> uint9 {
  //bus values are direct CRAM entry indexes:
  //d0-d3 => color (0 = neither background nor sprite)
  //d4-d7 => palette
  //d8    => source (0 = background; 1 = sprite)
  auto bus0 = vdp.vdc0.bus();
  auto bus1 = vdp.vdc1.bus();

  bool window0 = window[0] >= 64 && (window[0] - 64) >= hcounter / 2;
  bool window1 = window[1] >= 64 && (window[1] - 64) >= hcounter / 2;

  uint2 mode = !window0 << 0 | !window1 << 1;
  bool enableVDC0 = settings[mode].enableVDC0 && bus0.bit(0,3);
  bool enableVDC1 = settings[mode].enableVDC1 && bus1.bit(0,3);
  auto priority = settings[mode].priority;

  if(priority == 0 || priority == 3) {
    //SP0 > BG0 > SP1 > BG1
    if(bus0.bit(8) == 1 && enableVDC0) return bus0;
    if(bus0.bit(8) == 0 && enableVDC0) return bus0;
    if(bus1.bit(8) == 1 && enableVDC1) return bus1;
    if(bus1.bit(8) == 0 && enableVDC1) return bus1;
  }

  if(priority == 1) {
    //SP0 > SP1 > BG0 > BG1
    if(bus0.bit(8) == 1 && enableVDC0) return bus0;
    if(bus1.bit(8) == 1 && enableVDC1) return bus1;
    if(bus0.bit(8) == 0 && enableVDC0) return bus0;
    if(bus1.bit(8) == 0 && enableVDC1) return bus1;
  }

  if(priority == 2) {
    //BG0 > SP1 > BG1 > SP0
    if(bus0.bit(8) == 0 && enableVDC0) return bus0;
    if(bus1.bit(8) == 1 && enableVDC1) return bus1;
    if(bus1.bit(8) == 0 && enableVDC1) return bus1;
    if(bus0.bit(8) == 1 && enableVDC0) return bus0;
  }

  return 0x000;
}

auto VPC::read(uint5 address) -> uint8 {
  if(address >= 0x00 && address <= 0x07) return vdp.vdc0.read(address);
  if(address >= 0x10 && address <= 0x17) return vdp.vdc1.read(address);
  if(address >= 0x18 && address <= 0x1f) return 0xff;

  uint8 data = 0x00;

  if(address == 0x08) {
    data.bit(0)   = settings[0].enableVDC0;
    data.bit(1)   = settings[0].enableVDC1;
    data.bit(2,3) = settings[0].priority;
    data.bit(4)   = settings[1].enableVDC0;
    data.bit(5)   = settings[1].enableVDC1;
    data.bit(6,7) = settings[1].priority;
    return data;
  }

  if(address == 0x09) {
    data.bit(0)   = settings[2].enableVDC0;
    data.bit(1)   = settings[2].enableVDC1;
    data.bit(2,3) = settings[2].priority;
    data.bit(4)   = settings[3].enableVDC0;
    data.bit(5)   = settings[3].enableVDC1;
    data.bit(6,7) = settings[3].priority;
    return data;
  }

  if(address == 0x0a) {
    data.bit(0,7) = window[0].bit(0,7);
    return data;
  }

  if(address == 0x0b) {
    data.bit(0,1) = window[0].bit(8,9);
    return data;
  }

  if(address == 0x0c) {
    data.bit(0,7) = window[1].bit(0,7);
    return data;
  }

  if(address == 0x0d) {
    data.bit(0,1) = window[1].bit(8,9);
    return data;
  }

  if(address == 0x0e) {
    //select is not readable
    return data;
  }

  if(address == 0x0f) {
    //unused
    return data;
  }

  unreachable;
}

auto VPC::write(uint5 address, uint8 data) -> void {
  if(address >= 0x00 && address <= 0x07) return vdp.vdc0.write(address, data);
  if(address >= 0x10 && address <= 0x17) return vdp.vdc1.write(address, data);
  if(address >= 0x18 && address <= 0x1f) return;

  if(address == 0x08) {
    settings[0].enableVDC0 = data.bit(0);
    settings[0].enableVDC1 = data.bit(1);
    settings[0].priority   = data.bit(2,3);
    settings[1].enableVDC0 = data.bit(4);
    settings[1].enableVDC1 = data.bit(5);
    settings[1].priority   = data.bit(6,7);
    return;
  }

  if(address == 0x09) {
    settings[2].enableVDC0 = data.bit(0);
    settings[2].enableVDC1 = data.bit(1);
    settings[2].priority   = data.bit(2,3);
    settings[3].enableVDC0 = data.bit(4);
    settings[3].enableVDC1 = data.bit(5);
    settings[3].priority   = data.bit(6,7);
    return;
  }

  if(address == 0x0a) {
    window[0].bit(0,7) = data.bit(0,7);
    return;
  }

  if(address == 0x0b) {
    window[0].bit(8,9) = data.bit(0,1);
    return;
  }

  if(address == 0x0c) {
    window[1].bit(0,7) = data.bit(0,7);
    return;
  }

  if(address == 0x0d) {
    window[1].bit(8,9) = data.bit(0,1);
    return;
  }

  if(address == 0x0e) {
    select = data.bit(0);
    return;
  }

  if(address == 0x0f) {
    //unused
    return;
  }
}

auto VPC::store(uint2 address, uint8 data) -> void {
  if(select == 0) return vdp.vdc0.write(address, data);
  if(select == 1) return vdp.vdc1.write(address, data);
}

auto VPC::power() -> void {
  settings[0] = {};
  settings[1] = {};
  settings[2] = {};
  settings[3] = {};
  window[0] = 0;
  window[1] = 0;
  select = 0;
}
