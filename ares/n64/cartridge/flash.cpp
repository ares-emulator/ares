static constexpr u32 FlashMs = 187'500;

const Cartridge::Flash::Model Cartridge::Flash::models[] = {
  {"MX29L0000",   0x00c2, 0x0000, true,  85 * FlashMs,  85 * FlashMs, (3500 * FlashMs) / 1000},
  {"MX29L0001",   0x00c2, 0x0001, true,  85 * FlashMs,  85 * FlashMs, (3500 * FlashMs) / 1000},
  {"MX29L1100",   0x00c2, 0x001e, true,  85 * FlashMs,  85 * FlashMs, (3500 * FlashMs) / 1000},
  {"MX29L1101_A", 0x00c2, 0x001d, false, 85 * FlashMs,  85 * FlashMs, (3500 * FlashMs) / 1000},
  {"MX29L1101_B", 0x00c2, 0x0084, false, 85 * FlashMs,  85 * FlashMs, (3500 * FlashMs) / 1000},
  {"MX29L1101_C", 0x00c2, 0x008e, false, 85 * FlashMs,  85 * FlashMs, (3500 * FlashMs) / 1000},
  {"MN63F81MPN",  0x0032, 0x00f1, false, 280 * FlashMs, 300 * FlashMs, (300 * FlashMs) / 1000},
};

auto Cartridge::Flash::setModel(string name) -> void {
  model = &models[3];
  if(!name || name == "Unknown") return;
  for(auto& entry : models) {
    if(name == entry.name) { model = &entry; return; }
  }
}

auto Cartridge::Flash::power(bool) -> void {
  queue.remove(Queue::Flash_Complete);
  mode = Mode::ReadArray;
  status.data = 0;
  status.wsmReady() = 1;
  if(macronix()) status.ok() = 3;
  eraseSetup = EraseSetup::None;
  eraseSector = 0;
  busy = Busy::None;
  memory::fill<u8>(pageBuffer, sizeof(pageBuffer), 0xff);
  piOffset = 0;
  cirHighValid = 0;
  cirHigh = 0;
  openBus = 0;
  statusStale = 0;
  statusStaleValue = 0;
  pendingModeCmd = 0;
  pendingModeCount = 0;
  burstIndex = 0;
}

auto Cartridge::Flash::arrayOffset(u32 flashOffset) const -> u32 {
  if(model->wordIndexed) return flashOffset << 1;
  return flashOffset;
}

auto Cartridge::Flash::siliconHalf(u32 halfIndex) const -> u16 {
  u16 id[4] = {0x1111, 0x8001, model->manufacturerId, model->deviceId};
  if(halfIndex < 4) return id[halfIndex];
  if(matsushita()) return id[3];
  return id[halfIndex & 3];
}

auto Cartridge::Flash::piAddress(u32 address, PIDeviceTiming) -> bool {
  if(address < 0x0800'0000 || address > 0x0fff'ffff) return false;
  piOffset = address - 0x0800'0000;
  cirHighValid = 0;
  burstIndex = 0;
  return true;
}

auto Cartridge::Flash::piReadHalf(PIDeviceTiming) -> maybe<u16> {
  if(openBus) return nothing;

  if(mode == Mode::Status) {
    if(piOffset & 0x2'0000) {
      openBus = 1;
      return nothing;
    }
    if(statusStale) {
      statusStale = 0;
      u16 data = statusStaleValue;
      piOffset += 2;
      burstIndex++;
      return data;
    }
    u16 data = status.data;
    statusStaleValue = data;
    piOffset += 2;
    burstIndex++;
    return data;
  }

  if(mode == Mode::SiliconID) {
    if((piOffset & 0x2'0000) && matsushita()) {
      openBus = 1;
      return nothing;
    }
    u16 data = siliconHalf(burstIndex);
    statusStaleValue = data;
    piOffset += 2;
    burstIndex++;
    return data;
  }

  if(mode == Mode::LoadBytePage) {
    u32 index = (piOffset & 0x7f) & ~1;
    u16 data = pageBuffer[index] << 8 | pageBuffer[index + 1];
    statusStaleValue = data;
    piOffset += 2;
    return data;
  }

  u32 mask = wrapMask();
  u32 offset = arrayOffset(piOffset);
  u16 data = 0;
  if(offset < size) data = Memory::Writable::read<Half>(offset);
  statusStaleValue = data;
  if(model->wordIndexed) {
    piOffset = (piOffset & ~mask) | ((piOffset + 1) & mask);
  } else {
    piOffset = (piOffset & ~mask) | ((piOffset + 2) & mask);
  }
  return data;
}

auto Cartridge::Flash::piWriteHalf(u16 data, PIDeviceTiming) -> void {
  u32 offset = piOffset & ~1;

  if(offset == 0x1'0000 || offset == 0x1'0002) {
    if(!cirHighValid) {
      cirHigh = data;
      cirHighValid = 1;
      piOffset += 2;
      return;
    }
    cirHighValid = 0;
    piOffset += 2;
    command((u32)cirHigh << 16 | data);
    return;
  }

  if(mode == Mode::Status) {
    if(matsushita()) {
      status.ok() = 0;
    } else if(macronix() && offset == 0 && data == 0) {
      status.ok() = 3;
    }
    piOffset += 2;
    return;
  }

  if(mode == Mode::LoadBytePage) {
    u32 index = offset & 0x7f;
    if(matsushita()) {
      pageBuffer[index    ] &= data >> 8;
      pageBuffer[index + 1] &= data >> 0;
    } else {
      pageBuffer[index    ] = data >> 8;
      pageBuffer[index + 1] = data >> 0;
    }
    piOffset += 2;
    return;
  }

  piOffset += 2;
}

auto Cartridge::Flash::command(u32 data) -> void {
  u8 cmd = data >> 24;
  openBus = 0;
  cartridge.debugger.flash(data);

  if(busy != Busy::None) return;

  if(macronix() && cmd == 0xd2) {
    if(pendingModeCmd != cmd) {
      pendingModeCmd = cmd;
      pendingModeCount = 1;
      return;
    }
    pendingModeCount++;
    if(pendingModeCount < 2) return;
  }
  pendingModeCmd = 0;
  pendingModeCount = 0;

  switch(cmd) {
  case 0x3c: //ChipEraseSetup
    eraseSetup = EraseSetup::Chip;
    return;

  case 0x4b: //SectorEraseSetup
    eraseSetup = EraseSetup::Sector;
    eraseSector = (data & 0x3ff) >> 7;
    return;

  case 0x78: { //Erase
    if(eraseSetup == EraseSetup::None) return;
    u32 duration = model->sectorEraseClocks;
    if(eraseSetup == EraseSetup::Chip) {
      duration = model->chipEraseClocks;
      for(u32 address = 0; address < size; address += 2) {
        Memory::Writable::write<Half>(address, 0xffff);
      }
    }
    if(eraseSetup == EraseSetup::Sector) {
      u32 base = (u32)eraseSector << 14;
      for(u32 address = 0; address < 0x4000; address += 2) {
        Memory::Writable::write<Half>(base + address, 0xffff);
      }
    }
    eraseSetup = EraseSetup::None;
    mode = Mode::Status;
    status.wsmReady() = 0;
    status.eraseBusy() = 1;
    statusStale = macronix();
    busy = Busy::Erase;
    cpu.queueInsert(Queue::Flash_Complete, duration);
    return;
  }

  case 0xa5: { //ProgramPage
    u32 page = data & 0x3ff;
    u32 base = page << 7;
    for(u32 index = 0; index < 128; index++) {
      u8 value = Memory::Writable::read<Byte>(base + index);
      Memory::Writable::write<Byte>(base + index, value & pageBuffer[index]);
    }
    memory::fill<u8>(pageBuffer, sizeof(pageBuffer), 0xff);
    mode = Mode::Status;
    status.wsmReady() = 0;
    status.programBusy() = 1;
    statusStale = macronix();
    busy = Busy::Program;
    cpu.queueInsert(Queue::Flash_Complete, model->programClocks);
    return;
  }

  case 0xb4: //LoadBytePage
    mode = Mode::LoadBytePage;
    return;

  case 0xd2: //Status
    mode = Mode::Status;
    statusStale = macronix();
    return;

  case 0xe1: //SiliconID
    mode = Mode::SiliconID;
    return;

  case 0xf0: //ReadArray
    mode = Mode::ReadArray;
    return;

  default:
    debug(unusual, "[Cartridge::Flash::command] command=", hex(cmd, 2L));
    return;
  }
}

auto Cartridge::Flash::finish() -> void {
  status.wsmReady() = 1;
  status.busy() = 0;
  if(busy == Busy::Erase) {
    if(matsushita()) status.eraseOk() = 1;
  }
  if(busy == Busy::Program) {
    if(matsushita()) status.programOk() = 1;
  }
  busy = Busy::None;
}

auto Cartridge::Flash::serialize(serializer& s) -> void {
  Memory::Writable::serialize(s);
  s((u32&)mode);
  s(status);
  s((u32&)eraseSetup);
  s(eraseSector);
  s((u32&)busy);
  s(std::span<u8>{pageBuffer, sizeof(pageBuffer)});
  s(piOffset);
  s(cirHighValid);
  s(cirHigh);
  s(openBus);
  s(statusStale);
  s(statusStaleValue);
  s(pendingModeCmd);
  s(pendingModeCount);
  s(burstIndex);
}
