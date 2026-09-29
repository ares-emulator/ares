auto SC64::Host::sendPacket(u8 id, const std::vector<u8>& data) -> void {
  if(self.hostMode == HostMode::Unknown) {
    self.hostPendingPackets.push_back({id, data});
    return;
  }

  std::vector<u8> packet;
  auto append = [&](u32 value) {
    packet.push_back(value >> 24);
    packet.push_back(value >> 16);
    packet.push_back(value >> 8);
    packet.push_back(value >> 0);
  };
  if(self.hostMode == HostMode::Remote) {
    append(3);  // sc64deployer's remote transport: DataType::Packet
    packet.push_back(id);
  } else {
    packet.insert(packet.end(), {'P', 'K', 'T', id});
  }
  append(data.size());
  packet.insert(packet.end(), data.begin(), data.end());
  sendData(packet.data(), packet.size());
}

auto SC64::Host::onConnect() -> void {
}

auto SC64::Host::onDisconnect() -> void {
}

auto SC64::syncHostConnection() -> void {
  auto connected = host.hasClient();
  if(connected == hostConnected) return;

  hostConnected = connected;
  print(connected ? "SC64 USB host connected\n" : "SC64 USB host disconnected\n");
  hostInput.clear();
  hostMode = HostMode::Unknown;
  hostPendingPackets.clear();
  resetUsb();
}

auto SC64::pollHost() -> void {
  auto now = chrono::microsecond();
  if(hostLastPoll && now - hostLastPoll > 250'000) {
    print("SC64 USB host poll gap: ", (u32)((now - hostLastPoll) / 1000), " ms\n");
  }
  hostLastPoll = now;

  syncHostConnection();
  // The receive thread marks this atomically when it appends bytes. Avoid
  // taking the receive-buffer mutex on every emulated frame when the bridge
  // is idle; packet callbacks still run on the emulation thread.
  if(host.hasReceivedData()) host.poll();
  syncHostConnection();
  if(!usbInput.empty() && chrono::microsecond() >= usbInputDeadline) {
    print("SC64 USB input expired: type=0x", hex(usbInput.front().type, 2),
      " queued=", usbInput.size(), "\n");
    // The real SC64 expires the currently pending host packet when the N64
    // side does not acknowledge it in time.  Host USB writes are received
    // through a FIFO, so later packets must remain available after the
    // expired packet is discarded.
    usbInput.pop_front();
    usbInputOffset = 0;
    usbInputDeadline = usbInput.empty() ? 0 : chrono::microsecond() + 1'000'000;
    if(usbInput.empty()) registers.scr &= ~(1u << 25);
    host.sendPacket('G', {});
    updateInterrupt();
  }
}

auto SC64::open(string location, bool readOnly_, u32 hostPort_) -> bool {
  close();

  readOnly = readOnly_;
  sdInserted = false;
  if(location) {
    auto mode = readOnly ? file::mode::read : file::mode::modify;
    if(image.open(location, mode) && image.size() != 0 && image.size() % 512 == 0) {
      sdInserted = true;
      sectorCount = image.size() / 512;
    } else {
      image.close();
    }
  }

  hostPort = hostPort_ <= 65535 ? hostPort_ : 0;
  if(hostPort) host.open(hostPort, true);
  enabled = sdInserted || hostPort != 0;
  if(!enabled) return false;

  sdram.allocate(64 * 1024 * 1024, 0);
  if(self.rom.size) memcpy(sdram.data, self.rom.data, min<u32>(self.rom.size, sdram.size));
  // Keep the complete BlockRAM window available to the host protocol. The
  // guest-visible data buffer is the first 8 KiB; the remaining bytes cover
  // the EEPROM/64DD/FlashRAM windows used by SC64deployer.
  buffer.allocate(0x2d00, 0);
  sdInitialized = false;
  sdBlockAddressed = false;
  sdClock50MHz = false;
  byteSwap = false;
  unlocked = false;
  keySequence = 0;
  configs[0] = 1;
  configs[1] = 0;
  configs[2] = 0;
  configs[3] = 0;
  configs[4] = 0;
  configs[5] = 0;
  configs[6] = self.ram ? 3 : 0;
  configs[7] = 0xffff;
  configs[8] = 3;
  configs[9] = 0;
  configs[10] = 0;
  configs[11] = 0;
  configs[12] = 0;
  configs[13] = 0;
  configs[14] = 0;
  settings[0] = 1;
  resetUsb();
  registers = {};
  registers.identifier = 0x53437632;
  registers.scr = (1u << 28) | (1u << 26);
  return true;
}

auto SC64::close() -> void {
  pi.sc64Interrupt = 0;
  pi.updateInterrupt();
  hostConnected = false;
  host.close(false);
  flush();
  image.close();
  sdram.reset();
  buffer.reset();
  enabled = false;
  sdInserted = false;
  sdInitialized = false;
  sdBlockAddressed = false;
  sdClock50MHz = false;
  unlocked = false;
  keySequence = 0;
  byteSwap = false;
  hostPort = 0;
  hostInput.clear();
  usbInput.clear();
  usbInputOffset = 0;
  usbInputDeadline = 0;
  sectorCount = 0;
  usbOutputBusy = false;
  hostLastPoll = 0;
}

auto SC64::flush() -> void {
  if(image) image.flush();
}

auto SC64::piAddress(u32 address_, PIDeviceTiming timing) -> bool {
  if(!enabled || !timing.fasterThan({0, 0, 0})) return false;
  address_ &= ~1;

  if(unlocked && address_ >= 0x1ffe'0000 && address_ < 0x1ffe'2000) {
    target = Target::Buffer;
    offset = address_ - 0x1ffe'0000;
    return true;
  }

  if(unlocked && address_ >= 0x1380'0000 && address_ < 0x1400'0000) {
    target = Target::USBMemory;
    offset = address_ - 0x1380'0000;
    return true;
  }

  if(unlocked && address_ >= 0x1fff'0000 && address_ < 0x1fff'001c) {
    target = Target::Registers;
    offset = address_ - 0x1fff'0000;
    return true;
  }

  if(address_ == 0x1fff'0010) {
    target = Target::Registers;
    offset = 0x10;
    return true;
  }

  return false;
}

auto SC64::piReadHalf(PIDeviceTiming) -> maybe<u16> {
  if(target == Target::Buffer) {
    if(offset >= 0x2000) return nothing;
    auto data = buffer.read<Half>(offset);
    offset += 2;
    return (u16)data;
  }

  if(target == Target::USBMemory) {
    if(u64(0x0380'0000) + offset + 2 > sdram.size) return nothing;
    auto data = (u16(sdram.data[0x0380'0000 + offset + 0]) << 8)
              | (u16(sdram.data[0x0380'0000 + offset + 1]) << 0);
    offset += 2;
    return data;
  }

  if(target == Target::Registers) {
    auto address_ = offset & ~3;
    auto data = registerRead(address_);
    auto result = (offset & 2) ? (u16)data : (u16)(data >> 16);
    offset += 2;
    return result;
  }

  return nothing;
}

auto SC64::piWriteHalf(u16 data, PIDeviceTiming) -> void {
  if(target == Target::Buffer) {
    if(offset < 0x2000) buffer.write<Half>(offset, data);
    offset += 2;
    return;
  }

  if(target == Target::USBMemory) {
    // The debug window is readable at all times, but writes are enabled by
    // CONFIG_ROM_WRITE (config 1) on the real SC64.
    if(romWriteEnabled() && u64(0x0380'0000) + offset + 2 <= sdram.size) {
      sdram.data[0x0380'0000 + offset + 0] = data >> 8;
      sdram.data[0x0380'0000 + offset + 1] = data >> 0;
    }
    offset += 2;
    return;
  }

  if(target == Target::Registers) {
    auto address_ = offset & ~3;
    if(offset & 2) registerWrite(address_, pendingRegister | data);
    else pendingRegister = u32(data) << 16;
    offset += 2;
  }
}

auto SC64::registerRead(u32 address_) -> u32 {
  switch(address_) {
  case 0x00: return registers.scr;
  case 0x04: return registers.data0;
  case 0x08: return registers.data1;
  case 0x0c: return registers.identifier;
  case 0x14: return registers.irq;
  case 0x18: return registers.aux;
  default: return 0;
  }
}

auto SC64::registerWrite(u32 address_, u32 data) -> void {
  switch(address_) {
  case 0x00:
    registers.scr = (registers.scr & 0xff00'0000) | (data & 0x0000'01ff);
    if((data & 0xff) != 0) execute(data & 0xff);
    break;
  case 0x04: registers.data0 = data; break;
  case 0x08: registers.data1 = data; break;
  case 0x10:
    if(data == 0) {
      keySequence = 0;
      unlocked = false;
    } else if(data == 0x5f55'4e4c) {
      keySequence = 1;
    } else if(data == 0x4f43'4b5f && keySequence == 1) {
      keySequence = 0;
      unlocked = true;
    } else if(data == 0xffff'ffff) {
      keySequence = 0;
      unlocked = false;
      registers.scr &= ~((1u << 29) | (1u << 27) | (1u << 25) | (1u << 23)
                       | (1u << 28) | (1u << 26) | (1u << 24) | (1u << 22));
      updateInterrupt();
    } else {
      keySequence = 0;
    }
    break;
  case 0x14:
    if(data & (1u << 31)) registers.scr &= ~(1u << 29);
    if(data & (1u << 30)) registers.scr &= ~(1u << 27);
    if(data & (1u << 29)) registers.scr &= ~(1u << 25);
    if(data & (1u << 28)) registers.scr &= ~(1u << 23);
    if(data & (1u << 11)) registers.scr &= ~(1u << 24);
    if(data & (1u << 10)) registers.scr |=  (1u << 24);
    if(data & (1u << 9))  registers.scr &= ~(1u << 22);
    if(data & (1u << 8))  registers.scr |=  (1u << 22);
    registers.irq = data;
    updateInterrupt();
    break;
  case 0x18: {
    registers.aux = data;
    std::vector<u8> payload{
      (u8)(data >> 24), (u8)(data >> 16), (u8)(data >> 8), (u8)data
    };
    registers.scr |= 1u << 23;
    host.sendPacket('X', payload);
    updateInterrupt();
    break;
  }
  }
}

auto SC64::resetUsb() -> void {
  usbInput.clear();
  usbInputOffset = 0;
  usbInputDeadline = 0;
  hostInput.clear();
  registers.scr &= ~((1u << 25) | (1u << 23));
  usbOutputBusy = false;
  updateInterrupt();
}

auto SC64::enqueueUsbInput(u8 type, const std::vector<u8>& data) -> bool {
  if(data.size() > 0x00ff'ffff) return false;
  if(usbInput.empty()) usbInputDeadline = chrono::microsecond() + 1'000'000;
  usbInput.push_back({type, data});
  registers.scr |= 1u << 25;
  updateInterrupt();
  return true;
}

auto SC64::usbReadStatus() const -> u32 {
  if(usbInput.empty()) return 0;
  // USB_READ_STATUS returns the datatype in the low byte of response 0;
  // response 1 carries the remaining byte count.
  // Bit 31 reports an in-progress N64-side DMA read. Our command execution
  // is synchronous, so there is no observable interval in which it is set.
  return usbInput.front().type;
}

auto SC64::piMemoryAddress(u32 address_, u32 length) -> u8* {
  if(u64(address_) + length <= 0x1ffe'2000 && address_ >= 0x1ffe'0000)
    return buffer.data + (address_ - 0x1ffe'0000);
  if(u64(address_) + length <= 0x1400'0000 && address_ >= 0x1380'0000)
    return sdram.data + 0x0380'0000 + (address_ - 0x1380'0000);
  if(u64(address_) + length <= u64(0x1000'0000) + self.rom.size && address_ >= 0x1000'0000)
    return self.rom.data + (address_ - 0x1000'0000);
  return nullptr;
}

auto SC64::readPiMemory(u32 address_, u8* data, u32 size) -> bool {
  if(u64(address_) + size <= 0x0080'0000) {
    for(u32 index : range(size))
      data[index] = rdram.ram.read<Byte>(address_ + index, RBusDevice::ARES_DEBUGGER);
    return true;
  }
  auto source = piMemoryAddress(address_, size);
  if(!source) return false;
  memcpy(data, source, size);
  return true;
}

auto SC64::writePiMemory(u32 address_, const u8* data, u32 size) -> bool {
  if(u64(address_) + size <= 0x0080'0000) {
    for(u32 index : range(size))
      rdram.ram.write<Byte>(address_ + index, data[index], RBusDevice::ARES_DEBUGGER);
    return true;
  }
  if(address_ >= 0x1380'0000 && u64(address_) + size <= 0x1400'0000) {
    memcpy(sdram.data + 0x0380'0000 + (address_ - 0x1380'0000), data, size);
    return true;
  }
  if(address_ >= 0x1ffe'0000 && u64(address_) + size <= 0x1ffe'2000) {
    memcpy(buffer.data + (address_ - 0x1ffe'0000), data, size);
    return true;
  }
  return false;
}

auto SC64::hostMemoryAddress(u32 address_, u32 length) -> u8* {
  if(u64(address_) + length <= 0x0400'0000 && address_ >= 0x0380'0000)
    return sdram.data + address_;
  if(u64(address_) + length <= 0x0500'2d00 && address_ >= 0x0500'0000)
    return buffer.data + (address_ - 0x0500'0000);
  if(u64(address_) + length <= sdram.size && address_ < sdram.size)
    return sdram.data + address_;
  return nullptr;
}

auto SC64::readMemory(u32 address_, u8* data, u32 size) -> bool {
  auto source = hostMemoryAddress(address_, size);
  if(!source) return false;
  memcpy(data, source, size);
  return true;
}

auto SC64::writeMemory(u32 address_, const u8* data, u32 size) -> bool {
  auto target = hostMemoryAddress(address_, size);
  if(!target) return false;
  if(address_ >= 0x04e0'0000 && address_ < 0x04fe'0000) return false;
  memcpy(target, data, size);
  return true;
}

auto SC64::usbWrite(u8 type, u32 address_, u32 length) -> bool {
  if(length > 0x00ff'ffff) return false;
  std::vector<u8> payload(length);
  if(!readPiMemory(address_, payload.data(), length)) return false;

  std::vector<u8> packet(4 + length);
  packet[0] = type;
  packet[1] = length >> 16;
  packet[2] = length >> 8;
  packet[3] = length;
  memcpy(packet.data() + 4, payload.data(), length);
  host.sendPacket('U', packet);
  return true;
}

auto SC64::usbRead(u32 address_, u32 length) -> bool {
  if(usbInput.empty()) return false;
  auto& packet = usbInput.front();
  auto remaining = packet.data.size() - usbInputOffset;
  if(length > remaining || !writePiMemory(address_, packet.data.data() + usbInputOffset, length)) return false;

  usbInputOffset += length;
  if(usbInputOffset == packet.data.size()) {
    usbInput.pop_front();
    usbInputOffset = 0;
    usbInputDeadline = usbInput.empty() ? 0 : chrono::microsecond() + 1'000'000;
  }
  if(usbInput.empty()) {
    registers.scr &= ~(1u << 25);
  }
  updateInterrupt();
  return true;
}

auto SC64::updateInterrupt() -> void {
  // SCR stores each pending bit immediately above its corresponding mask
  // bit.  They cannot be intersected directly: USB pending is bit 25 while
  // USB enable is bit 24, for example.
  auto interrupt = ((registers.scr >> 1) & registers.scr)
                & ((1u << 28) | (1u << 26) | (1u << 24) | (1u << 22));
  pi.sc64Interrupt = interrupt;
  pi.updateInterrupt();
}

auto SC64::hostResponse(u8 command, bool error, const std::vector<u8>& data) -> void {
  std::vector<u8> packet;
  auto append = [&](u32 value) {
    packet.push_back(value >> 24);
    packet.push_back(value >> 16);
    packet.push_back(value >> 8);
    packet.push_back(value >> 0);
  };
  if(hostMode == HostMode::Remote) {
    append(2);  // sc64deployer's remote transport: DataType::Response
    packet.push_back(command);
    packet.push_back(error ? 1 : 0);
  } else {
    packet.insert(packet.end(), error ? std::initializer_list<u8>{'E', 'R', 'R', command}
                                      : std::initializer_list<u8>{'C', 'M', 'P', command});
  }
  append(data.size());
  packet.insert(packet.end(), data.begin(), data.end());
  host.send(packet);
}

auto SC64::hostConfigGet(u32 id, u32& value) const -> bool {
  if(id >= 15) return false;
  value = configs[id];
  return true;
}

auto SC64::hostConfigSet(u32 id, u32 value, u32& previous) -> bool {
  if(id >= 15 || id == 12) return false;
  previous = configs[id];
  configs[id] = value;
  return true;
}

auto SC64::hostTransfer(bool write, u32 address_, u32 sector, u32 count) -> u32 {
  if(!sdInserted) return 1;       // SD_ERROR_NO_CARD_IN_SLOT
  if(!sdInitialized) return 2;    // SD_ERROR_NOT_INITIALIZED
  if(!count || count > 0x7fffff) return 3; // SD_ERROR_INVALID_ARGUMENT
  if(u64(sector) >= sectorCount || count > sectorCount - sector) return 4;
  if(!hostMemoryAddress(address_, u64(count) * 512)) return 4; // SD_ERROR_INVALID_ADDRESS

  u8 data[512];
  for(u32 index : range(count)) {
    if(write) {
      if(!readMemory(address_ + index * 512, data, sizeof(data))) return 4;
      setByteSwap(data, sizeof(data));
      image.seek((u64(sector) + index) * 512);
      for(auto byte : data) image.write(byte);
    } else {
      image.seek((u64(sector) + index) * 512);
      for(auto& byte : data) byte = image.read();
      setByteSwap(data, sizeof(data));
      if(!writeMemory(address_ + index * 512, data, sizeof(data))) return 4;
    }
  }
  if(write) flush();
  return 0;
}

auto SC64::hostCommand(u8 command, u32 data0, u32 data1, const std::vector<u8>& data) -> void {
  auto word = [](u32 value) {
    return std::vector<u8>{(u8)(value >> 24), (u8)(value >> 16), (u8)(value >> 8), (u8)value};
  };

  switch(command) {
  case 'v':
    hostResponse(command, false, {0x53, 0x43, 0x76, 0x32});
    return;
  case 'V':
    hostResponse(command, false, {0, 2, 0, 20, 0, 0, 0, 0});
    return;
  case 'R':
    configs[0] = 1;
    configs[1] = 0;
    configs[2] = 0;
    configs[3] = 0;
    configs[4] = 0;
    configs[5] = 0;
    configs[6] = self.ram ? 3 : 0;
    configs[7] = 0xffff;
    configs[8] = 3;
    configs[9] = 0;
    configs[10] = 0;
    configs[11] = 0;
    configs[12] = 0;
    configs[13] = 0;
    configs[14] = 0;
    resetUsb();
    hostResponse(command, false, {});
    return;
  case 'B':
    hostResponse(command, false, {});
    return;
  case 'c': {
    u32 value = 0;
    if(!hostConfigGet(data0, value)) hostResponse(command, true, {});
    else hostResponse(command, false, word(value));
    return;
  }
  case 'C': {
    u32 previous = 0;
    if(!hostConfigSet(data0, data1, previous)) hostResponse(command, true, {});
    else hostResponse(command, false, {});
    return;
  }
  case 'a':
    if(data0 != 0) hostResponse(command, true, {});
    else hostResponse(command, false, word(settings[0]));
    return;
  case 'A':
    if(data0 != 0) hostResponse(command, true, {});
    else {
      settings[0] = data1;
      hostResponse(command, false, {});
    }
    return;
  case 't':
    {
      auto info = chrono::local::timeinfo();
      auto weekday = info.weekday ? info.weekday : 7;  // Monday=1, Sunday=7
      auto century = info.year / 100;
      century = century >= 19 ? century - 19 : 0;
      hostResponse(command, false, {
        BCD::encode(weekday), BCD::encode(info.hour), BCD::encode(info.minute), BCD::encode(info.second),
        BCD::encode(century), BCD::encode(info.year % 100), BCD::encode(info.month), BCD::encode(info.day)
      });
    }
    return;
  case 'T':
    hostResponse(command, false, {});
    return;
  case 'i': {
    u32 result = 0;
    switch(data1) {
    case 0:
      sdInitialized = false;
      sdBlockAddressed = false;
      sdClock50MHz = false;
      byteSwap = false;
      break;
    case 1:
      if(!sdInserted) result = 1;
      else {
        sdInitialized = true;
        sdBlockAddressed = true;
        sdClock50MHz = true;
      }
      break;
    case 2: break;
    case 3:
      if(!sdInitialized) result = 2;
      else {
        auto info = sdInfo();
        if(!writeMemory(data0, info.data(), info.size())) result = 4;
      }
      break;
    case 4:
      if(!sdInitialized) result = 2;
      else byteSwap = true;
      break;
    case 5:
      if(!sdInitialized) result = 2;
      else byteSwap = false;
      break;
    default: result = 5; break;
    }
    auto response = word(result);
    auto status = word(sdStatus());
    response.insert(response.end(), status.begin(), status.end());
    hostResponse(command, result != 0, response);
    return;
  }
  case 's':
  case 'S': {
    if(data.size() != 4) {
      hostResponse(command, true, word(3));
      return;
    }
    auto sector = (u32(data[0]) << 24) | (u32(data[1]) << 16) | (u32(data[2]) << 8) | data[3];
    auto result = hostTransfer(command == 'S', data0, sector, data1);
    hostResponse(command, result != 0, word(result));
    return;
  }
  case 'D':
  case 'W':
    hostResponse(command, false, {});
    return;
  case 'p':
    hostResponse(command, false, word(0x10000));
    return;
  case '?':
    hostResponse(command, false, {0, 0, 0, 0, 0, 0, 0, 0});
    return;
  case '%':
    hostResponse(command, false, {0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});
    return;
  case 'P':
  case 'f':
  case 'F':
  case 'K':
    hostResponse(command, true, {});
    return;
  case 'm': {
    std::vector<u8> response(data1);
    if(data1 && !readMemory(data0, response.data(), data1)) {
      hostResponse(command, true, {});
      return;
    }
    hostResponse(command, false, response);
    return;
  }
  case 'M':
    if(data.size() != data1 || !writeMemory(data0, data.data(), data1)) {
      hostResponse(command, true, {});
      return;
    }
    hostResponse(command, false, {});
    return;
  case 'X': {
    registers.aux = data0;
    registers.scr |= 1u << 23;
    updateInterrupt();
    hostResponse(command, false, {});
    return;
  }
  case 'U':
    if(data1 == 0 || data1 != data.size() || !enqueueUsbInput(data0 & 0xff, data)) return;
    return;
  default:
    hostResponse(command, true, {});
    return;
  }
}

auto SC64::hostCommandDataLength(u8 command, u32 data1) const -> u32 {
  // The SC64 wire protocol has no explicit payload length in its CMD
  // header.  Commands that carry data derive it from arg1.
  switch(command) {
  case 'M':  // MEMORY_WRITE
  case 'U':  // USB_WRITE
    return data1;
  case 's':  // SD_READ carries the starting sector as a 32-bit data word.
  case 'S':  // SD_WRITE carries the starting sector as a 32-bit data word.
    return 4;
  default:
    return 0;
  }
}

auto SC64::hostDataDirect() -> void {
  auto readWord = [&](u32 offset) {
    return (u32(hostInput[offset + 0]) << 24) | (u32(hostInput[offset + 1]) << 16)
         | (u32(hostInput[offset + 2]) << 8) | u32(hostInput[offset + 3]);
  };

  while(true) {
    if(hostInput.size() < 12) return;
    if(hostInput[0] != 'C' || hostInput[1] != 'M' || hostInput[2] != 'D') {
      host.disconnectClient();
      hostInput.clear();
      return;
    }

    auto command = hostInput[3];
    auto data0 = readWord(4);
    auto data1 = readWord(8);
    auto length = hostCommandDataLength(command, data1);
    if(length > 16 * 1024 * 1024) {
      host.disconnectClient();
      hostInput.clear();
      return;
    }
    auto total = u64(12) + length;
    if(hostInput.size() < total) return;

    std::vector<u8> payload(hostInput.begin() + 12, hostInput.begin() + total);
    hostInput.erase(hostInput.begin(), hostInput.begin() + total);
    hostCommand(command, data0, data1, payload);
  }
}

auto SC64::hostDataRemote() -> void {
  auto readWord = [&](u32 offset) {
    return (u32(hostInput[offset + 0]) << 24) | (u32(hostInput[offset + 1]) << 16)
         | (u32(hostInput[offset + 2]) << 8) | u32(hostInput[offset + 3]);
  };

  while(true) {
    // DataType::Command (u32), command byte, two arguments, payload length.
    if(hostInput.size() < 17) return;

    auto type = readWord(0);
    if(type == 0xcafe'beef) {
      // KeepAlive has no body. It is normally sent by the deployer server,
      // but accepting it here keeps the transport symmetric.
      hostInput.erase(hostInput.begin(), hostInput.begin() + 4);
      continue;
    }
    if(type != 1) {
      host.disconnectClient();
      hostInput.clear();
      return;
    }

    auto command = hostInput[4];
    auto data0 = readWord(5);
    auto data1 = readWord(9);
    auto length = readWord(13);
    if(length > 16 * 1024 * 1024) {
      host.disconnectClient();
      hostInput.clear();
      return;
    }
    auto total = u64(17) + length;
    if(hostInput.size() < total) return;

    std::vector<u8> payload(hostInput.begin() + 17, hostInput.begin() + total);
    hostInput.erase(hostInput.begin(), hostInput.begin() + total);
    hostCommand(command, data0, data1, payload);
  }
}

auto SC64::hostData(const std::vector<u8>& data) -> void {
  hostInput.insert(hostInput.end(), data.begin(), data.end());

  if(hostMode == HostMode::Unknown) {
    if(hostInput.size() >= 3 && hostInput[0] == 'C' && hostInput[1] == 'M' && hostInput[2] == 'D') {
      hostMode = HostMode::Direct;
    } else if(hostInput.size() >= 4
           && hostInput[0] == 0 && hostInput[1] == 0
           && hostInput[2] == 0 && hostInput[3] == 1) {
      hostMode = HostMode::Remote;
    } else if(hostInput.size() >= 4) {
      host.disconnectClient();
      hostInput.clear();
      return;
    }
  }

  if(hostMode != HostMode::Unknown && !hostPendingPackets.empty()) {
    auto pending = std::move(hostPendingPackets);
    hostPendingPackets.clear();
    for(auto& packet : pending) host.sendPacket(packet.type, packet.data);
  }

  if(hostMode == HostMode::Direct) hostDataDirect();
  if(hostMode == HostMode::Remote) hostDataRemote();
}

auto SC64::Host::onData(const std::vector<u8>& data) -> void {
  self.hostData(data);
}

// Match the status word returned by the SC64 SD controller.  The emulated
// image is always an inserted card; the remaining bits track the state
// maintained by the command interface.
auto SC64::sdStatus() const -> u32 {
  return (byteSwap ? 1u << 4 : 0u)
       | (sdClock50MHz ? 1u << 3 : 0u)
       | (sdBlockAddressed ? 1u << 2 : 0u)
       | (sdInitialized ? 1u << 1 : 0u)
       | (sdInserted ? 1u : 0u);
}

auto SC64::sdInfo() const -> std::vector<u8> {
  // The hardware exposes CSD followed by CID (16 bytes each).  The image is
  // modeled as an SDHC card, so synthesize the CSD capacity from its sector
  // count; the CID is intentionally stable but otherwise opaque.
  std::vector<u8> info(32);
  info[0] = 0x40;  // CSD structure version 2.0
  u64 csize = sectorCount >= 1024 ? (sectorCount / 1024) - 1 : 0;
  csize = min<u64>(csize, 0x3f'ffff);
  info[7] = (u8)(csize >> 16) & 0x3f;
  info[8] = (u8)(csize >> 8);
  info[9] = (u8)csize;
  return info;
}

auto SC64::writeSdInfo(u32 address_) -> bool {
  if(!sdInitialized) return false;
  auto info = sdInfo();

  if(u64(address_) + info.size() <= 0x1ffe'2000 && address_ >= 0x1ffe'0000) {
    for(u32 index : range(info.size())) buffer.write<Byte>(address_ - 0x1ffe'0000 + index, info[index]);
    return true;
  }
  if(u64(address_) + info.size() <= 0x0080'0000) {
    for(u32 index : range(info.size()))
      rdram.ram.write<Byte>(address_ + index, info[index], RBusDevice::ARES_DEBUGGER);
    return true;
  }
  return false;
}

auto SC64::complete() -> void {
  registers.scr &= ~(1u << 31);
  if(registers.scr & (1u << 8)) registers.scr |= 1u << 27;
  updateInterrupt();
}

auto SC64::error(u32 code) -> void {
  if(code) registers.data0 = code;
  registers.scr |= 1u << 30;
  complete();
}

auto SC64::execute(u8 command) -> void {
  registers.scr |= 1u << 31;
  registers.scr &= ~(1u << 30);

  switch(command) {
  case 'v':
    registers.data0 = registers.identifier;
    complete();
    return;
  case 'V':
    registers.data0 = (2 << 16) | 20;
    registers.data1 = 0;
    complete();
    return;
  case 't':
    {
      auto info = chrono::local::timeinfo();
      auto weekday = info.weekday ? info.weekday : 7;
      auto century = info.year / 100;
      century = century >= 19 ? century - 19 : 0;
      registers.data0 = (u32(BCD::encode(weekday)) << 24)
                      | (u32(BCD::encode(info.hour)) << 16)
                      | (u32(BCD::encode(info.minute)) << 8)
                      | u32(BCD::encode(info.second));
      registers.data1 = (u32(BCD::encode(century)) << 24)
                      | (u32(BCD::encode(info.year % 100)) << 16)
                      | (u32(BCD::encode(info.month)) << 8)
                      | u32(BCD::encode(info.day));
    }
    complete();
    return;
  case 'T':
    // Ares has no separate SC64 battery-backed RTC; accept synchronization
    // commands so deployer clients can continue their session.
    complete();
    return;
  case 'c':
    if(!hostConfigGet(registers.data0, registers.data1)) { error(1); return; }
    complete();
    return;
  case 'C': {
    u32 previous = 0;
    if(!hostConfigSet(registers.data0, registers.data1, previous)) { error(1); return; }
    registers.data1 = previous;
    complete();
    return;
  }
  case 'u':
    registers.data0 = usbReadStatus();
    registers.data1 = usbInput.empty() ? 0 : (u32)(usbInput.front().data.size() - usbInputOffset);
    complete();
    return;
  case 'm':
    if(!usbRead(registers.data0, registers.data1)) error(4);
    else complete();
    return;
  case 'M':
    if(!usbWrite(registers.data1 >> 24, registers.data0, registers.data1 & 0x00ff'ffff)) error(4);
    else complete();
    return;
  case 'U':
    registers.data0 = usbOutputBusy ? (1u << 31) : 0;
    complete();
    return;
  case 'i':
    {
    u32 result = 0;
    switch(registers.data1) {
    case 0:
      sdInitialized = false;
      sdBlockAddressed = false;
      sdClock50MHz = false;
      byteSwap = false;
      break;
    case 1:
      if(!sdInserted) result = 1;
      else {
        sdInitialized = true;
        sdBlockAddressed = true;
        sdClock50MHz = true;
      }
      break;
    case 2: break;
    case 3:
      if(!writeSdInfo(registers.data0)) result = sdInitialized ? 4 : 2;
      break;
    case 4: byteSwap = true; break;
    case 5: byteSwap = false; break;
    default: result = 5; break; // SD_ERROR_INVALID_OPERATION
    }
    registers.data0 = result;
    registers.data1 = sdStatus();
    if(result) { error(); return; }
    complete();
    return;
    }
  case 'I':
    sdSector = registers.data0;
    complete();
    return;
  case 's':
    if(!sdInserted) error(1);
    else if(!sdInitialized) error(2);
    else if(!transfer(false, registers.data0, registers.data1)) error(4);
    else { sdSector += registers.data1; complete(); }
    return;
  case 'S':
    if(readOnly) error(4);
    else if(!sdInserted) error(1);
    else if(!sdInitialized) error(2);
    else if(!transfer(true, registers.data0, registers.data1)) error(4);
    else { sdSector += registers.data1; flush(); complete(); }
    return;
  default:
    error(5);
    return;
  }
}

auto SC64::address(u32 value, u32 length) -> u8* {
  if(u64(value) + length <= 0x1ffe'2000 && value >= 0x1ffe'0000)
    return buffer.data + (value - 0x1ffe'0000);
  if(u64(value) + length <= u64(0x1000'0000) + self.rom.size && value >= 0x1000'0000)
    return self.rom.data + (value - 0x1000'0000);
  return nullptr;
}

auto SC64::setByteSwap(u8* data, u32 size) -> void {
  if(!byteSwap) return;
  for(u32 address_ = 0; address_ < size; address_ += 4)
    std::swap(data[address_ + 0], data[address_ + 3]), std::swap(data[address_ + 1], data[address_ + 2]);
}

auto SC64::transfer(bool write, u32 piAddress, u32 count) -> bool {
  if(!count || count > 0x7fffff || sdSector >= sectorCount || count > sectorCount - sdSector) return false;
  auto bytes = u64(count) * 512;
  auto target = address(piAddress, bytes);
  bool dram = piAddress < 0x0080'0000 && u64(piAddress) + bytes <= 0x0080'0000;
  bool bufferTarget = piAddress >= 0x1ffe'0000 && u64(piAddress) + bytes <= 0x1ffe'2000;
  if(!target && !dram) return false;

  u8 sector[512];
  for(u32 n : range(count)) {
    image.seek((u64(sdSector) + n) * 512);
    if(write) {
      for(u32 index : range(512)) {
        auto address_ = piAddress + n * 512 + index;
        sector[index] = dram ? rdram.ram.read<Byte>(address_, RBusDevice::ARES_DEBUGGER)
          : bufferTarget ? buffer.read<Byte>(address_ - 0x1ffe'0000) : target[n * 512 + index];
      }
      setByteSwap(sector, sizeof sector);
      for(auto byte : sector) image.write(byte);
    } else {
      for(auto& byte : sector) byte = image.read();
      setByteSwap(sector, sizeof sector);
      for(u32 index : range(512)) {
        auto address_ = piAddress + n * 512 + index;
        if(dram) rdram.ram.write<Byte>(address_, sector[index], RBusDevice::ARES_DEBUGGER);
        else if(bufferTarget) buffer.write<Byte>(address_ - 0x1ffe'0000, sector[index]);
        else target[n * 512 + index] = sector[index];
      }
    }
  }
  return true;
}
