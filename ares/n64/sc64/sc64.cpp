#include <n64/n64.hpp>
#include <nall/tcptext/tcp-socket.hpp>
#include <deque>
#include <vector>

namespace ares::Nintendo64 {

SC64 sc64;

auto SC64::Host::sendPacket(PacketId id, const std::vector<u8>& data) -> void {
  // If no client is attached, drop the packet.
  // Queue packets only while a client is connected
  // but its framing is not yet known.
  if(!self.hostConnected) return;
  if(self.hostMode == HostMode::Unknown) {
    self.hostPendingPackets.push_back({(u8)id, data});
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
    append((u32)RemoteType::Packet);
    packet.push_back((u8)id);
  } else {
    packet.insert(packet.end(), {'P', 'K', 'T', (u8)id});
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
  syncHostConnection();
  // Do not take the receive-buffer mutex while the link is idle.
  // Callbacks run on the emulation thread.
  if(host.hasReceivedData()) host.poll();
  syncHostConnection();

  auto now = chrono::microsecond();
  if(hostMode == HostMode::Remote && hostConnected) {
    if(!hostLastKeepAlive) hostLastKeepAlive = now;
    if(now - hostLastKeepAlive >= KeepAliveInterval) {
      host.send({0xca, 0xfe, 0xbe, 0xef});  // RemoteType::KeepAlive
      hostLastKeepAlive = now;
    }
  } else {
    hostLastKeepAlive = 0;
  }

  if(!usbInput.empty() && now >= usbInputDeadline) {
    // The head packet expires when the N64 does not read it in time.
    // Later packets stay in the queue.
    usbInput.pop_front();
    usbInputOffset = 0;
    usbInputDeadline = usbInput.empty() ? 0 : chrono::microsecond() + UsbInputTimeout;
    if(usbInput.empty()) registers.scr &= ~Scr::UsbIrqPending;
    host.sendPacket(PacketId::DataFlushed, {});
    updateInterrupt();
    hostDataDispatch();
  }

  pollIsViewer();
}

// ISViewer support is not bus hardware. The cart polls an IS64 ring buffer
// in SDRAM at the configured address. It sends new bytes to the USB host.
auto SC64::pollIsViewer() -> void {
  auto address = config(Config::IsvAddress);
  if(!address) return;
  // A buffer near the end of SDRAM extends into flash space. Flash is not emulated.
  if(u64(address) + IsvWindowSize > sdram.size) return;

  auto get = [&](u32 offset) -> u32 {
    return (u32(sdram.data[offset + 0]) << 24) | (u32(sdram.data[offset + 1]) << 16)
         | (u32(sdram.data[offset + 2]) << 8) | u32(sdram.data[offset + 3]);
  };
  auto set = [&](u32 offset, u32 value) -> void {
    sdram.data[offset + 0] = value >> 24;
    sdram.data[offset + 1] = value >> 16;
    sdram.data[offset + 2] = value >> 8;
    sdram.data[offset + 3] = value >> 0;
  };

  // Reply to the setup token from the game with the
  // PI address of the buffer and a ready marker.
  if(get(IsvSetupToken) == IsvToken) {
    set(IsvSetupToken, 0);
    set(IsvSetupAddress, address | PiSdramBase);
    set(IsvSetupReady, IsvToken);
    return;
  }

  if(get(address + 0x00) != IsvToken) return;
  constexpr u32 bufferSize = IsvWindowSize - IsvBufferOffset;
  auto readPointer = get(address + IsvReadPointer);
  auto writePointer = get(address + IsvWritePointer);
  if(readPointer >= bufferSize || writePointer >= bufferSize) return;
  if(readPointer == writePointer) return;

  bool wrap = writePointer < readPointer;
  auto length = (wrap ? bufferSize : writePointer) - readPointer;
  // If no client is attached, the cart drops the bytes.
  // The read pointer moves forward in both cases.
  if(hostConnected && hostMode != HostMode::Unknown) {
    std::vector<u8> payload(length);
    memcpy(payload.data(), sdram.data + address + IsvBufferOffset + readPointer, length);
    host.sendPacket(PacketId::IsvOutput, payload);
  }
  set(address + IsvReadPointer, wrap ? 0 : writePointer);
}

auto SC64::resetConfigs() -> void {
  config(Config::BootloaderSwitch) = 1;
  resetConfigState();
  settings[0] = 1;  // LED enable
}

// The state-reset command from the host also uses this function.
// That command keeps the bootloader switch and the LED setting.
auto SC64::resetConfigState() -> void {
  config(Config::RomWriteEnable) = 0;
  config(Config::RomShadowEnable) = 0;
  config(Config::DdMode) = 0;
  config(Config::IsvAddress) = 0;
  config(Config::BootMode) = 0;
  // The save type is already known from the loaded cartridge's save hardware.
  SaveType saveType = SaveType::None;
  if(cartridge.eeprom.size == 512) saveType = SaveType::Eeprom4k;
  else if(cartridge.eeprom.size == 2048) saveType = SaveType::Eeprom16k;
  else if(cartridge.ram.size == 32_KiB) saveType = SaveType::Sram;
  else if(cartridge.flash) saveType = SaveType::Flashram;
  else if(cartridge.ram.size == 96_KiB) saveType = SaveType::SramBanked;
  else if(cartridge.ram.size >= 128_KiB) saveType = SaveType::Sram1M;
  config(Config::SaveType) = (u32)saveType;
  config(Config::CicSeed) = 0xffff;  // automatic
  config(Config::TvType) = 3;  // passthrough
  config(Config::DdSdEnable) = 0;
  config(Config::DdDriveType) = 0;
  config(Config::DdDiskState) = 0;
  config(Config::ButtonState) = 0;
  config(Config::ButtonMode) = 0;
  config(Config::RomExtendedEnable) = 0;
}

auto SC64::open(string location, bool readOnly_, u32 hostPort_) -> void {
  close();

  readOnly = readOnly_;
  sdInserted = false;
  if(location) {
    auto mode = readOnly ? file::mode::read : file::mode::modify;
    if(image.open(location, mode) && image.size() != 0 && image.size() % SectorSize == 0) {
      sdInserted = true;
      sectorCount = image.size() / SectorSize;
    } else {
      image.close();
    }
  }

  hostPort = hostPort_ <= 65535 ? hostPort_ : 0;
  if(hostPort) host.open(hostPort, true);

  sdram.allocate(SdramSize, 0);
  if(cartridge.rom.size) {
    auto length = min<u32>(cartridge.rom.size, sdram.size);
    for(u32 index : range(length)) sdram.data[index] = cartridge.rom.read<Byte>(index);
  }
  buffer.allocate(BlockRamSize, 0);
  sdInitialized = false;
  sdBlockAddressed = false;
  sdClock50MHz = false;
  byteSwap = false;
  unlocked = false;
  keySequence = 0;
  resetConfigs();
  resetUsb();
  registers = {};
  registers.scr = Scr::BtnIrqMask | Scr::CmdIrqMask;
}

auto SC64::close() -> void {
  registers = {};
  updateInterrupt();
  hostConnected = false;
  host.close(false);
  flush();
  image.close();
  sdram.reset();
  buffer.reset();
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
}

auto SC64::power(bool reset) -> void {
  // BlockRAM is serialized unconditionally, so keep it allocated even when the cart is disabled.
  if(!buffer) buffer.allocate(BlockRamSize, 0);

  unlocked = false;
  keySequence = 0;
  registers.scr &= ~(Scr::BtnIrqPending | Scr::CmdIrqPending | Scr::UsbIrqPending
                   | Scr::AuxIrqPending | Scr::UsbIrqMask | Scr::AuxIrqMask
                   | Scr::CmdIrqRequest);
  // While reset is active, the cart releases the N64 side of the SD lock.
  sdReleaseLock(SdLock::N64);
  target = Target::None;
  offset = 0;
  pendingRegister = 0;

  if(!reset) {
    // A power cycle restarts the cart.
    sdInitialized = false;
    sdBlockAddressed = false;
    sdClock50MHz = false;
    byteSwap = false;
    sdLock = SdLock::None;
    sdSector = 0;
    resetConfigs();
    registers.data0 = 0;
    registers.data1 = 0;
    registers.aux = 0;
    registers.scr = Scr::BtnIrqMask | Scr::CmdIrqMask;
    resetUsb();
  }

  updateInterrupt();
}

auto SC64::flush() -> void {
  if(image) image.flush();
}

auto SC64::piAddress(u32 address_, PIDeviceTiming timing) -> bool {
  if(!timing.fasterThan({0, 0, 0})) return false;
  address_ &= ~1;

  // BlockRAM is visible only when the cart is unlocked.
  if(unlocked && address_ >= PiBlockRamBase && address_ < PiBlockRamBase + BlockRamSize) {
    target = Target::Buffer;
    offset = address_ - PiBlockRamBase;
    return true;
  }

  // All of the PI ROM window maps to SDRAM. Writes also require
  // ROM_WRITE_ENABLE (piWriteHalf).
  if(address_ >= PiSdramBase && address_ < PiSdramBase + SdramSize) {
    target = Target::Sdram;
    offset = address_ - PiSdramBase;
    return true;
  }

  if(unlocked && address_ >= PiRegisterBase && address_ < PiRegisterBase + RegisterAreaSize) {
    target = Target::Registers;
    offset = address_ - PiRegisterBase;
    return true;
  }

  // The key register accepts the unlock sequence when the cart is locked.
  if(address_ == PiRegisterBase + (u32)Register::Key) {
    target = Target::Registers;
    offset = (u32)Register::Key;
    return true;
  }

  return false;
}

auto SC64::piReadHalf(PIDeviceTiming) -> maybe<u16> {
  if(target == Target::Buffer) {
    if(offset >= BlockRamSize) return nothing;
    auto data = (u16(buffer.data[offset + 0]) << 8)
              | (u16(buffer.data[offset + 1]) << 0);
    offset += 2;
    return data;
  }

  if(target == Target::Sdram) {
    if(u64(offset) + 2 > sdram.size) return nothing;
    auto data = (u16(sdram.data[offset + 0]) << 8)
              | (u16(sdram.data[offset + 1]) << 0);
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
    // The FlashRAM section at the top of BlockRAM is read-only.
    if(offset < FlashBufferOffset) {
      buffer.data[offset + 0] = data >> 8;
      buffer.data[offset + 1] = data >> 0;
    }
    offset += 2;
    return;
  }

  if(target == Target::Sdram) {
    // ROM_WRITE_ENABLE controls writes. Reads are always permitted.
    if(romWriteEnabled() && u64(offset) + 2 <= sdram.size) {
      sdram.data[offset + 0] = data >> 8;
      sdram.data[offset + 1] = data >> 0;
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
  switch((Register)address_) {
  case Register::Scr:        return registers.scr;
  case Register::Data0:      return registers.data0;
  case Register::Data1:      return registers.data1;
  case Register::Identifier: return registers.identifier;
  case Register::Irq:        return 0;
  case Register::Aux:        return registers.aux;
  default:                   return 0;
  }
}

auto SC64::registerWrite(u32 address_, u32 data) -> void {
  switch((Register)address_) {
  case Register::Scr:
    registers.scr = (registers.scr & ~(Scr::CmdIrqRequest | Scr::CommandMask))
                  | (data & (Scr::CmdIrqRequest | Scr::CommandMask));
    if((data & Scr::CommandMask) != 0) execute(data & Scr::CommandMask);
    break;
  case Register::Data0: registers.data0 = data; break;
  case Register::Data1: registers.data1 = data; break;
  case Register::Key:
    if(data == KeyReset) {
      keySequence = 0;
    } else if(data == KeyUnlock1) {
      keySequence = 1;
    } else if(data == KeyUnlock2 && keySequence == 1) {
      keySequence = 0;
      unlocked = true;
    } else if(data == KeyLock) {
      keySequence = 0;
      unlocked = false;
      registers.scr &= ~(Scr::BtnIrqPending | Scr::CmdIrqPending | Scr::UsbIrqPending
                       | Scr::AuxIrqPending | Scr::UsbIrqMask | Scr::AuxIrqMask);
      updateInterrupt();
    } else {
      keySequence = 0;
    }
    break;
  case Register::Irq:
    if(data & Irq::BtnClear) registers.scr &= ~Scr::BtnIrqPending;
    if(data & Irq::CmdClear) registers.scr &= ~Scr::CmdIrqPending;
    if(data & Irq::UsbClear) registers.scr &= ~Scr::UsbIrqPending;
    if(data & Irq::AuxClear) registers.scr &= ~Scr::AuxIrqPending;
    if(data & Irq::UsbEnable)  registers.scr |=  Scr::UsbIrqMask;
    if(data & Irq::UsbDisable) registers.scr &= ~Scr::UsbIrqMask;
    if(data & Irq::AuxEnable)  registers.scr |=  Scr::AuxIrqMask;
    if(data & Irq::AuxDisable) registers.scr &= ~Scr::AuxIrqMask;
    updateInterrupt();
    break;
  case Register::Aux: {
    registers.aux = data;
    std::vector<u8> payload{
      (u8)(data >> 24), (u8)(data >> 16), (u8)(data >> 8), (u8)data
    };
    host.sendPacket(PacketId::AuxData, payload);
    break;
  }
  default: break;
  }
}

auto SC64::resetUsb() -> void {
  usbInput.clear();
  usbInputOffset = 0;
  usbInputDeadline = 0;
  hostInput.clear();
  registers.scr &= ~(Scr::UsbIrqPending | Scr::AuxIrqPending);
  usbOutputBusy = false;
  updateInterrupt();
}

auto SC64::enqueueUsbInput(u8 type, const std::vector<u8>& data) -> bool {
  if(data.size() > UsbLengthMask) return false;
  if(usbInput.empty()) usbInputDeadline = chrono::microsecond() + UsbInputTimeout;
  usbInput.push_back({type, data});
  registers.scr |= Scr::UsbIrqPending;
  updateInterrupt();
  return true;
}

auto SC64::usbReadStatus() const -> u32 {
  if(usbInput.empty()) return 0;
  // Bit 31 (DMA busy) is never set. Command execution is synchronous.
  return usbInput.front().type;
}

// Address translation for the N64 side. The PI ROM window maps to SDRAM.
// The data buffer and the EEPROM/64DD sections map to BlockRAM.
// Flash is not currently emulated.
auto SC64::piMemoryAddress(u32 address_, u32 length) -> u8* {
  if(u64(address_) + length <= PiBlockRamBase + DataBufferSize && address_ >= PiBlockRamBase)
    return buffer.data + (address_ - PiBlockRamBase);
  if(u64(address_) + length <= PiBlockRamBase + McuBufferOffset && address_ >= PiBlockRamBase + EepromOffset)
    return buffer.data + (address_ - PiBlockRamBase);
  if(u64(address_) + length <= PiSdramBase + SdramSize && address_ >= PiSdramBase)
    return sdram.data + (address_ - PiSdramBase);
  return nullptr;
}

auto SC64::readPiMemory(u32 address_, u8* data, u32 size) -> bool {
  auto source = piMemoryAddress(address_, size);
  if(!source) return false;
  memcpy(data, source, size);
  return true;
}

auto SC64::writePiMemory(u32 address_, const u8* data, u32 size) -> bool {
  auto target = piMemoryAddress(address_, size);
  if(!target) return false;
  memcpy(target, data, size);
  return true;
}

auto SC64::hostMemoryAddress(u32 address_, u32 length) -> u8* {
  if(u64(address_) + length <= HostBlockRamBase + BlockRamSize && address_ >= HostBlockRamBase)
    return buffer.data + (address_ - HostBlockRamBase);
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
  memcpy(target, data, size);
  return true;
}

auto SC64::usbWrite(u8 type, u32 address_, u32 length) -> bool {
  if(length > UsbLengthMask) return false;
  std::vector<u8> payload(length);
  if(!readPiMemory(address_, payload.data(), length)) return false;

  std::vector<u8> packet(4 + length);
  packet[0] = type;
  packet[1] = length >> 16;
  packet[2] = length >> 8;
  packet[3] = length;
  memcpy(packet.data() + 4, payload.data(), length);
  host.sendPacket(PacketId::DebugOutput, packet);
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
  }
  usbInputDeadline = usbInput.empty() ? 0 : chrono::microsecond() + UsbInputTimeout;
  if(usbInput.empty()) {
    registers.scr &= ~Scr::UsbIrqPending;
  }
  updateInterrupt();
  if(usbInput.empty()) hostDataDispatch();
  return true;
}

auto SC64::irqLine() const -> bool {
  // Each pending bit is one position above its mask bit.
  auto interrupt = ((registers.scr >> 1) & registers.scr)
                & (Scr::BtnIrqMask | Scr::CmdIrqMask | Scr::UsbIrqMask | Scr::AuxIrqMask);
  return interrupt != 0;
}

auto SC64::updateInterrupt() -> void {
  // The INT line is shared between the cartridge and the DD.
  pollCartridgeInterrupt();
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
    append(2);  // remote transport: response
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
  if(id >= 15) return false;
  switch((Config)id) {
  case Config::ButtonState:  // read-only
    return false;
  case Config::DdMode:  // disabled/regs/IPL/full
    if(value > 3) return false;
    break;
  case Config::IsvAddress:  // word-aligned, in SDRAM
    if(value >= SdramSize || value % 4) return false;
    break;
  case Config::BootMode:  // menu/ROM/DD IPL/direct ROM/direct DD IPL
    if(value > 4) return false;
    break;
  case Config::SaveType:  // none through fake FlashRAM
    if(value > 7) return false;
    break;
  case Config::CicSeed:  // 0x00-0xff, or 0xffff for automatic
    if(value != 0xffff && value > 0xff) return false;
    break;
  case Config::TvType:  // PAL/NTSC/MPAL/passthrough
    if(value > 3) return false;
    break;
  case Config::DdDriveType:  // retail/development
    if(value > 1) return false;
    break;
  case Config::DdDiskState:  // ejected/inserted/changed
    if(value > 2) return false;
    break;
  case Config::ButtonMode:  // none/N64 IRQ/USB packet/DD disk swap
    if(value > 3) return false;
    break;
  default:
    break;
  }
  previous = configs[id];
  configs[id] = value;
  return true;
}

auto SC64::hostTransfer(bool write, u32 address_, u32 sector, u32 count) -> SdError {
  if(count > MaxSectorCount) return SdError::InvalidArgument;
  if(!count || !hostMemoryAddress(address_, u64(count) * SectorSize)) return SdError::InvalidAddress;
  auto lock = sdGetLock(SdLock::USB);
  if(lock != SdError::Ok) return lock;
  if(!sdInserted) return SdError::NoCardInSlot;
  if(!sdInitialized) return SdError::NotInitialized;
  if(write && readOnly) return SdError::Cmd25Io;
  if(u64(sector) >= sectorCount || count > sectorCount - sector) return SdError::InvalidAddress;

  u8 data[SectorSize];
  for(u32 index : range(count)) {
    if(write) {
      if(!readMemory(address_ + index * SectorSize, data, sizeof(data))) return SdError::InvalidAddress;
      image.seek((u64(sector) + index) * SectorSize);
      for(auto byte : data) image.write(byte);
    } else {
      image.seek((u64(sector) + index) * SectorSize);
      for(auto& byte : data) byte = image.read();
      if(address_ + index * SectorSize < HostBlockRamBase) setByteSwap(data, sizeof(data));
      if(!writeMemory(address_ + index * SectorSize, data, sizeof(data))) return SdError::InvalidAddress;
    }
  }
  if(write) flush();
  return SdError::Ok;
}

// Acquire the SD interface if it is free or if this side already holds it.
auto SC64::sdTryLock(SdLock lock) -> SdError {
  if(sdLock == SdLock::None) sdLock = lock;
  return sdGetLock(lock);
}

auto SC64::sdGetLock(SdLock lock) const -> SdError {
  return sdLock == lock ? SdError::Ok : SdError::Locked;
}

auto SC64::sdReleaseLock(SdLock lock) -> void {
  if(sdLock == lock) sdLock = SdLock::None;
}

auto SC64::hostCommand(u8 command, u32 data0, u32 data1, const std::vector<u8>& data) -> void {
  auto word = [](u32 value) {
    return std::vector<u8>{(u8)(value >> 24), (u8)(value >> 16), (u8)(value >> 8), (u8)value};
  };

  switch((HostCommand)command) {
  case HostCommand::IdentifierGet:
    hostResponse(command, false, word(registers.identifier));
    return;
  case HostCommand::VersionGet: {
    auto response = word(FirmwareVersion);
    auto revision = word(FirmwareRevision);
    response.insert(response.end(), revision.begin(), revision.end());
    hostResponse(command, false, response);
    return;
  }
  case HostCommand::StateReset:
    resetConfigState();
    sdReleaseLock(SdLock::USB);
    hostResponse(command, false, {});
    return;
  case HostCommand::CicParamsSet:
    // CIC parameters have no effect here. Acknowledge the command.
    hostResponse(command, false, {});
    return;
  case HostCommand::ConfigGet: {
    u32 value = 0;
    if(!hostConfigGet(data0, value)) hostResponse(command, true, word(data1));
    else hostResponse(command, false, word(value));
    return;
  }
  case HostCommand::ConfigSet: {
    u32 previous = 0;
    if(!hostConfigSet(data0, data1, previous)) hostResponse(command, true, {});
    else hostResponse(command, false, {});
    return;
  }
  case HostCommand::SettingGet:
    if(data0 != 0) hostResponse(command, true, word(data1));  // only the LED setting exists
    else hostResponse(command, false, word(settings[0]));
    return;
  case HostCommand::SettingSet:
    if(data0 != 0) hostResponse(command, true, {});
    else {
      settings[0] = data1;
      hostResponse(command, false, {});
    }
    return;
  case HostCommand::TimeGet:
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
  case HostCommand::TimeSet:
    hostResponse(command, false, {});
    return;
  case HostCommand::SdCardOp: {
    auto result = SdError::Ok;
    switch((SdOp)data1) {
    case SdOp::Deinit:
      result = sdTryLock(SdLock::USB);
      if(result == SdError::Ok) {
        sdInitialized = false;
        sdBlockAddressed = false;
        sdClock50MHz = false;
        byteSwap = false;
        sdReleaseLock(SdLock::USB);
      }
      break;
    case SdOp::Init:
      result = sdTryLock(SdLock::USB);
      if(result == SdError::Ok) {
        byteSwap = false;
        if(!sdInserted) {
          result = SdError::NoCardInSlot;
          sdReleaseLock(SdLock::USB);
        } else {
          sdInitialized = true;
          sdBlockAddressed = true;
          sdClock50MHz = true;
        }
      }
      break;
    case SdOp::GetStatus: break;
    case SdOp::GetInfo:
      // The address check comes before the lock check.
      if(!hostMemoryAddress(data0, SdInfoSize)) { result = SdError::InvalidAddress; break; }
      result = sdGetLock(SdLock::USB);
      if(result == SdError::Ok && !sdInitialized) result = SdError::NotInitialized;
      if(result == SdError::Ok) {
        auto info = sdInfo();
        if(!writeMemory(data0, info.data(), info.size())) result = SdError::InvalidAddress;
      }
      break;
    case SdOp::ByteSwapOn:
      result = sdGetLock(SdLock::USB);
      if(result == SdError::Ok && !sdInitialized) result = SdError::NotInitialized;
      if(result == SdError::Ok) byteSwap = true;
      break;
    case SdOp::ByteSwapOff:
      result = sdGetLock(SdLock::USB);
      if(result == SdError::Ok && !sdInitialized) result = SdError::NotInitialized;
      if(result == SdError::Ok) byteSwap = false;
      break;
    default: result = SdError::InvalidOperation; break;
    }
    auto response = word((u32)result);
    auto status = word(sdStatus());
    response.insert(response.end(), status.begin(), status.end());
    hostResponse(command, result != SdError::Ok, response);
    return;
  }
  case HostCommand::SdRead:
  case HostCommand::SdWrite: {
    if(data.size() != 4) {
      hostResponse(command, true, word((u32)SdError::InvalidArgument));
      return;
    }
    auto sector = (u32(data[0]) << 24) | (u32(data[1]) << 16) | (u32(data[2]) << 8) | data[3];
    auto result = hostTransfer((HostCommand)command == HostCommand::SdWrite, data0, sector, data1);
    hostResponse(command, result != SdError::Ok, word((u32)result));
    return;
  }
  case HostCommand::DdSetBlockReady:
  case HostCommand::WritebackEnable:
    // 64DD mapping and save writeback are not emulated.
    // Acknowledge the command.
    hostResponse(command, false, {});
    return;
  case HostCommand::FlashWaitBusy:
    hostResponse(command, false, word(FlashEraseBlockSize));
    return;
  case HostCommand::DebugGet:
    hostResponse(command, false, {0, 0, 0, 0, 0, 0, 0, 0});
    return;
  case HostCommand::DiagnosticGet:
    // Diagnostic marker and version.
    // Voltage and temperature are not emulated.
    hostResponse(command, false, {0x80, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});
    return;
  case HostCommand::FlashEraseBlock:
    // Flash is not emulated. Fail with no payload.
    hostResponse(command, true, {});
    return;
  case HostCommand::UpdateBackup:
    // Firmware updates are not emulated. Fail.
    hostResponse(command, true, {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff});
    return;
  case HostCommand::UpdatePrepare:
    // Firmware updates are not emulated. Fail.
    hostResponse(command, true, {0xff, 0xff, 0xff, 0xff});
    return;
  case HostCommand::MemoryRead: {
    std::vector<u8> response(data1);
    if(data1 && !readMemory(data0, response.data(), data1)) {
      hostResponse(command, true, {});
      return;
    }
    hostResponse(command, false, response);
    return;
  }
  case HostCommand::MemoryWrite:
    if(data.size() != data1 || !writeMemory(data0, data.data(), data1)) {
      hostResponse(command, true, {});
      return;
    }
    hostResponse(command, false, {});
    return;
  case HostCommand::AuxWrite: {
    registers.aux = data0;
    registers.scr |= Scr::AuxIrqPending;
    updateInterrupt();
    hostResponse(command, false, {});
    return;
  }
  case HostCommand::UsbWrite:
    if(data1 == 0 || data1 != data.size() || !enqueueUsbInput(data0 & 0xff, data)) return;
    return;
  default:
    // Unknown commands, and N64-only commands such as FlashProgram.
    hostResponse(command, true, {0xff, 0xff, 0xff, 0xff});
    return;
  }
}

auto SC64::hostCommandDataLength(u8 command, u32 data1) const -> u32 {
  // The direct framing has no payload length field.
  // Commands with data get the length from their arguments.
  switch((HostCommand)command) {
  case HostCommand::MemoryWrite:
  case HostCommand::UsbWrite:
    return data1;
  case HostCommand::SdRead:   // the start sector comes as a data word
  case HostCommand::SdWrite:
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

  // Header: "CMD", command byte, two argument words.
  constexpr u32 headerSize = 12;

  while(true) {
    if(!usbInput.empty()) return;
    if(hostInput.size() < headerSize) return;
    if(hostInput[0] != 'C' || hostInput[1] != 'M' || hostInput[2] != 'D') {
      host.disconnectClient();
      hostInput.clear();
      return;
    }

    auto command = hostInput[3];
    auto data0 = readWord(4);
    auto data1 = readWord(8);
    auto length = hostCommandDataLength(command, data1);
    if(length > MaxHostPayload) {
      host.disconnectClient();
      hostInput.clear();
      return;
    }
    auto total = u64(headerSize) + length;
    if(hostInput.size() < total) return;

    std::vector<u8> payload(hostInput.begin() + headerSize, hostInput.begin() + total);
    hostInput.erase(hostInput.begin(), hostInput.begin() + total);
    hostCommand(command, data0, data1, payload);
  }
}

auto SC64::hostDataRemote() -> void {
  auto readWord = [&](u32 offset) {
    return (u32(hostInput[offset + 0]) << 24) | (u32(hostInput[offset + 1]) << 16)
         | (u32(hostInput[offset + 2]) << 8) | u32(hostInput[offset + 3]);
  };

  // Header: type word, command byte, two argument words, payload length.
  constexpr u32 headerSize = 17;

  while(true) {
    if(!usbInput.empty()) return;
    if(hostInput.size() < headerSize) return;

    auto type = readWord(0);
    if(type == (u32)RemoteType::KeepAlive) {
      // A keep-alive has no body. Accept it to keep the transport symmetric.
      hostInput.erase(hostInput.begin(), hostInput.begin() + 4);
      continue;
    }
    if(type != (u32)RemoteType::Command) {
      host.disconnectClient();
      hostInput.clear();
      return;
    }

    auto command = hostInput[4];
    auto data0 = readWord(5);
    auto data1 = readWord(9);
    auto length = readWord(13);
    if(length > MaxHostPayload) {
      host.disconnectClient();
      hostInput.clear();
      return;
    }
    auto total = u64(headerSize) + length;
    if(hostInput.size() < total) return;

    std::vector<u8> payload(hostInput.begin() + headerSize, hostInput.begin() + total);
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

  hostDataDispatch();
}

auto SC64::hostDataDispatch() -> void {
  if(hostMode != HostMode::Unknown && !hostPendingPackets.empty()) {
    auto pending = std::move(hostPendingPackets);
    hostPendingPackets.clear();
    for(auto& packet : pending) host.sendPacket((PacketId)packet.type, packet.data);
  }

  if(hostMode == HostMode::Direct) hostDataDirect();
  if(hostMode == HostMode::Remote) hostDataRemote();
}

auto SC64::Host::onData(const std::vector<u8>& data) -> void {
  self.hostData(data);
}

auto SC64::sdStatus() const -> u32 {
  return (byteSwap ? 1u << 4 : 0u)
       | (sdClock50MHz ? 1u << 3 : 0u)
       | (sdBlockAddressed ? 1u << 2 : 0u)
       | (sdInitialized ? 1u << 1 : 0u)
       | (sdInserted ? 1u : 0u);
}

auto SC64::sdInfo() const -> std::vector<u8> {
  // CSD then CID, 16 bytes each.
  // The SDHC CSD is computed from the image size. The CID stays zero.
  std::vector<u8> info(SdInfoSize);
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
  return writePiMemory(address_, info.data(), info.size());
}

auto SC64::complete() -> void {
  registers.scr &= ~Scr::CmdBusy;
  if(registers.scr & Scr::CmdIrqRequest) registers.scr |= Scr::CmdIrqPending;
  updateInterrupt();
}

// DATA0 holds the error type and the error code. DATA1 is set to zero.
auto SC64::error(ErrorType type, u32 code) -> void {
  registers.data0 = (u32(type) << 24) | code;
  registers.data1 = 0;
  registers.scr |= Scr::CmdError;
  complete();
}

auto SC64::execute(u8 command) -> void {
  registers.scr |= Scr::CmdBusy;
  registers.scr &= ~Scr::CmdError;

  switch((Command)command) {
  case Command::IdentifierGet:
    registers.data0 = registers.identifier;
    complete();
    return;
  case Command::VersionGet:
    registers.data0 = FirmwareVersion;
    registers.data1 = FirmwareRevision;
    complete();
    return;
  case Command::TimeGet:
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
  case Command::TimeSet:
    // Time comes from the host clock. Accept time writes and do nothing.
    complete();
    return;
  case Command::ConfigGet:
    if(!hostConfigGet(registers.data0, registers.data1)) { error(CfgError::InvalidId); return; }
    complete();
    return;
  case Command::ConfigSet: {
    u32 previous = 0;
    if(!hostConfigSet(registers.data0, registers.data1, previous)) { error(CfgError::InvalidId); return; }
    registers.data1 = previous;
    complete();
    return;
  }
  case Command::SettingGet:
    // Only setting 0 (LED enable) exists.
    if(registers.data0 != 0) { error(CfgError::InvalidId); return; }
    registers.data1 = settings[0];
    complete();
    return;
  case Command::SettingSet:
    // Only setting 0 (LED enable) exists.
    if(registers.data0 != 0) { error(CfgError::InvalidId); return; }
    settings[0] = registers.data1;
    complete();
    return;
  case Command::UsbReadStatus:
    registers.data0 = usbReadStatus();
    registers.data1 = usbInput.empty() ? 0 : (u32)(usbInput.front().data.size() - usbInputOffset);
    complete();
    return;
  case Command::UsbRead:
    // The command fails only when the address does not translate.
    if(!usbRead(registers.data0, registers.data1)) error(CfgError::InvalidAddress);
    else complete();
    return;
  case Command::UsbWrite:
    // The command fails only when the address does not translate.
    if(!usbWrite(registers.data1 >> 24, registers.data0, registers.data1 & UsbLengthMask)) error(CfgError::InvalidAddress);
    else complete();
    return;
  case Command::UsbWriteStatus:
    registers.data0 = usbOutputBusy ? (1u << 31) : 0;
    complete();
    return;
  case Command::SdCardOp:
    {
    // Deinit and init acquire the N64 lock. Get-status does not use the
    // lock. Get-info checks its address first. The byte-swap operations
    // require the lock and an initialized card.
    auto result = SdError::Ok;
    switch((SdOp)registers.data1) {
    case SdOp::Deinit:
      result = sdTryLock(SdLock::N64);
      if(result == SdError::Ok) {
        sdInitialized = false;
        sdBlockAddressed = false;
        sdClock50MHz = false;
        byteSwap = false;
        sdReleaseLock(SdLock::N64);
      }
      break;
    case SdOp::Init:
      result = sdTryLock(SdLock::N64);
      if(result == SdError::Ok) {
        byteSwap = false;
        if(!sdInserted) {
          result = SdError::NoCardInSlot;
          sdReleaseLock(SdLock::N64);
        } else {
          sdInitialized = true;
          sdBlockAddressed = true;
          sdClock50MHz = true;
        }
      }
      break;
    case SdOp::GetStatus:
      registers.data1 = sdStatus();
      break;
    case SdOp::GetInfo:
      if(!piMemoryAddress(registers.data0, SdInfoSize)) { result = SdError::InvalidAddress; break; }
      result = sdGetLock(SdLock::N64);
      if(result == SdError::Ok && !sdInitialized) result = SdError::NotInitialized;
      if(result == SdError::Ok && !writeSdInfo(registers.data0)) result = SdError::InvalidAddress;
      break;
    case SdOp::ByteSwapOn:
      result = sdGetLock(SdLock::N64);
      if(result == SdError::Ok && !sdInitialized) result = SdError::NotInitialized;
      if(result == SdError::Ok) byteSwap = true;
      break;
    case SdOp::ByteSwapOff:
      result = sdGetLock(SdLock::N64);
      if(result == SdError::Ok && !sdInitialized) result = SdError::NotInitialized;
      if(result == SdError::Ok) byteSwap = false;
      break;
    default: result = SdError::InvalidOperation; break;
    }
    if(result != SdError::Ok) { error(result); return; }
    complete();
    return;
    }
  case Command::SdSectorSet: {
    auto lock = sdGetLock(SdLock::N64);
    if(lock != SdError::Ok) { error(lock); return; }
    sdSector = registers.data0;
    complete();
    return;
  }
  case Command::SdRead: {
    auto result = transfer(false, registers.data0, registers.data1);
    if(result != SdError::Ok) { error(result); return; }
    sdSector += registers.data1;
    complete();
    return;
  }
  case Command::SdWrite: {
    auto result = transfer(true, registers.data0, registers.data1);
    if(result != SdError::Ok) { error(result); return; }
    sdSector += registers.data1;
    complete();
    return;
  }
  case Command::DiskMappingSet:
    // 64DD disk mapping is not emulated. Accept and do nothing.
    complete();
    return;
  case Command::WritebackPending:
    // Save writeback is not emulated. It is never pending.
    registers.data0 = 0;
    complete();
    return;
  case Command::WritebackSdInfo:
    // Save writeback is not emulated. Accept and do nothing.
    complete();
    return;
  case Command::FlashProgram:
    // Flash is not emulated. Lengths that are too large are rejected
    // first. Then no address can translate into flash.
    if(registers.data1 >= DataBufferSize) { error(CfgError::InvalidArgument); return; }
    error(CfgError::InvalidAddress);
    return;
  case Command::FlashWaitBusy:
    // Flash operations complete immediately.
    // Report ready and the erase block size.
    registers.data0 = FlashEraseBlockSize;
    complete();
    return;
  case Command::FlashEraseBlock:
    // Flash is not emulated.
    error(CfgError::InvalidAddress);
    return;
  case Command::DiagnosticGet:
    // Only diagnostic 0 (voltage << 16 | temperature) exists. Voltage and
    // temperature are not emulated. Both fields read zero.
    if(registers.data0 != 0) { error(CfgError::InvalidId); return; }
    registers.data1 = 0;
    complete();
    return;
  default:
    error(CfgError::UnknownCommand);
    return;
  }
}

auto SC64::setByteSwap(u8* data, u32 size) -> void {
  if(!byteSwap) return;
  for(u32 address_ = 0; address_ < size; address_ += 2)
    std::swap(data[address_ + 0], data[address_ + 1]);
}

auto SC64::transfer(bool write, u32 piAddress, u32 count) -> SdError {
  if(count > MaxSectorCount) return SdError::InvalidArgument;
  auto bytes = u64(count) * SectorSize;
  auto target = piMemoryAddress(piAddress, bytes);
  if(!count || !target) return SdError::InvalidAddress;
  auto lock = sdGetLock(SdLock::N64);
  if(lock != SdError::Ok) return lock;
  if(!sdInserted) return SdError::NoCardInSlot;
  if(!sdInitialized) return SdError::NotInitialized;
  if(write && readOnly) return SdError::Cmd25Io;
  if(sdSector >= sectorCount || count > sectorCount - sdSector) return SdError::InvalidAddress;

  // The read byte-swap applies only to the SDRAM window.
  // It never applies to BlockRAM.
  bool swapEligible = piAddress >= PiSdramBase && piAddress < PiSdramBase + SdramSize;

  u8 sector[SectorSize];
  for(u32 n : range(count)) {
    image.seek((u64(sdSector) + n) * SectorSize);
    if(write) {
      memcpy(sector, target + n * SectorSize, sizeof(sector));
      for(auto byte : sector) image.write(byte);
    } else {
      for(auto& byte : sector) byte = image.read();
      if(swapEligible) setByteSwap(sector, sizeof(sector));
      memcpy(target + n * SectorSize, sector, sizeof(sector));
    }
  }
  if(write) flush();
  return SdError::Ok;
}

auto SC64::serialize(serializer& s) -> void {
  s(registers.scr);
  s(registers.data0);
  s(registers.data1);
  s(registers.aux);
  s(configs);
  s(settings);
  s(unlocked);
  s(keySequence);
  s(sdSector);
  s(sdInitialized);
  s(sdBlockAddressed);
  s(sdClock50MHz);
  s(byteSwap);
  s((u32&)sdLock);
  s((u32&)target);
  s(offset);
  s(pendingRegister);
  s(buffer);

  // sdram is intentionally not serialized.
  // SC64 SDRAM is too large for savestates or rewind.

  // USB state cannot be serialized reliably.
  if(s.reading()) {
    usbInput.clear();
    usbInputOffset = 0;
    usbInputDeadline = 0;
    usbOutputBusy = false;
    registers.scr &= ~Scr::UsbIrqPending;
    updateInterrupt();
  }
}

}
