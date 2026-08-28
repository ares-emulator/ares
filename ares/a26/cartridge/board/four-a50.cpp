struct FourA50 : Interface {
  using Interface::Interface;
  Memory::Readable<n8> rom;
  Memory::Writable<n8> ram;
  n16 sliceLow;
  n16 sliceMiddle;
  n16 sliceHigh;
  n1 romLow;
  n1 romMiddle;
  n1 romHigh;
  n16 lastAddress;
  n8 lastData;

  auto load() -> void override {
    Interface::load(rom, "program.rom");
  }

  auto read(n16 address, n8 data) -> n8 override {
    address &= 0x1fff;
    if(!address.bit(12)) {
      checkBankSwitch(address, data);
    } else if((address & 0x1800) == 0x1000) {
      data = romLow ? readRom(sliceLow + (address & 0x07ff)) : ram.read(sliceLow + (address & 0x07ff));
    } else if(address <= 0x1dff) {
      data = romMiddle ? readRom(0x10000 + sliceMiddle + (address & 0x07ff))
                       : ram.read(sliceMiddle + (address & 0x07ff));
    } else if(address <= 0x1eff) {
      data = romHigh ? readRom(0x10000 + sliceHigh + (address & 0x00ff))
                     : ram.read(sliceHigh + (address & 0x00ff));
    } else {
      data = readRom(0x1ff00 + (address & 0x00ff));
      if((lastData & 0xe0) == 0x60 && (lastAddress >= 0x1000 || lastAddress < 0x0200)) {
        sliceHigh = (sliceHigh & 0xf0ff) | ((address & 0x0008) << 8) | ((address & 0x0070) << 4);
      }
    }
    lastData = data;
    lastAddress = address;
    return data;
  }

  auto write(n16 address, n8 data) -> n8 override {
    address &= 0x1fff;
    if(!address.bit(12)) {
      checkBankSwitch(address, data);
    } else if((address & 0x1800) == 0x1000) {
      if(!romLow) {
        ram.write(sliceLow + (address & 0x07ff), data);
      }
    } else if(address <= 0x1dff) {
      if(!romMiddle) {
        ram.write(sliceMiddle + (address & 0x07ff), data);
      }
    } else if(address <= 0x1eff) {
      if(!romHigh) {
        ram.write(sliceHigh + (address & 0x00ff), data);
      }
    } else if((lastData & 0xe0) == 0x60 && (lastAddress >= 0x1000 || lastAddress < 0x0200)) {
      sliceHigh = (sliceHigh & 0xf0ff) | ((address & 0x0008) << 8) | ((address & 0x0070) << 4);
    }
    lastData = data;
    lastAddress = address;
    return data;
  }

  auto power(bool reset) -> void override {
    sliceLow = 0;
    sliceMiddle = 0;
    sliceHigh = 0;
    romLow = 1;
    romMiddle = 1;
    romHigh = 1;
    lastAddress = 0xffff;
    lastData = 0xff;
    ram.allocate(32_KiB, 0x00);
  }

  auto serialize(serializer& s) -> void override {
    s(ram);
    s(sliceLow);
    s(sliceMiddle);
    s(sliceHigh);
    s(romLow);
    s(romMiddle);
    s(romHigh);
    s(lastAddress);
    s(lastData);
  }

private:
  auto readRom(u32 address) -> n8 {
    return rom.read(address % rom.size());
  }

  auto bankROMLower(u16 value) -> void { romLow = 1; sliceLow = value << 11; }
  auto bankRAMLower(u16 value) -> void { romLow = 0; sliceLow = value << 11; }
  auto bankROMMiddle(u16 value) -> void { romMiddle = 1; sliceMiddle = value << 11; }
  auto bankRAMMiddle(u16 value) -> void { romMiddle = 0; sliceMiddle = value << 11; }
  auto bankROMHigh(u16 value) -> void { romHigh = 1; sliceHigh = value << 8; }
  auto bankRAMHigh(u16 value) -> void { romHigh = 0; sliceHigh = value << 8; }

  auto checkBankSwitch(n16 address, n8 data) -> void {
    if((lastData & 0xe0) == 0x60 && (lastAddress >= 0x1000 || lastAddress < 0x0200)) {
      if((address & 0x0f00) == 0x0c00) bankROMHigh(address & 0x00ff);
      else if((address & 0x0f00) == 0x0d00) bankRAMHigh(address & 0x007f);
      else if((address & 0x0f40) == 0x0e00) bankROMLower(address & 0x001f);
      else if((address & 0x0f40) == 0x0e40) bankRAMLower(address & 0x000f);
      else if((address & 0x0f40) == 0x0f00) bankROMMiddle(address & 0x001f);
      else if((address & 0x0f50) == 0x0f40) bankRAMMiddle(address & 0x000f);
      else if((address & 0x0f00) == 0x0400) sliceLow ^= 0x0800;
      else if((address & 0x0f00) == 0x0500) sliceLow ^= 0x1000;
      else if((address & 0x0f00) == 0x0800) sliceMiddle ^= 0x0800;
      else if((address & 0x0f00) == 0x0900) sliceMiddle ^= 0x1000;
    }

    if((address & 0x0f75) == 0x0074) bankROMHigh(data);
    else if((address & 0x0f75) == 0x0075) bankRAMHigh(data & 0x7f);
    else if((address & 0x0f7c) == 0x0078) {
      if((data & 0xf0) == 0x00) bankROMLower(data & 0x0f);
      else if((data & 0xf0) == 0x40) bankRAMLower(data & 0x0f);
      else if((data & 0xf0) == 0x90) bankROMMiddle((data & 0x0f) | 0x10);
      else if((data & 0xf0) == 0xc0) bankRAMMiddle(data & 0x0f);
    }
  }
};
