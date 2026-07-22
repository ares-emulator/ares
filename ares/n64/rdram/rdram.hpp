//RAMBUS RAM

struct RDRAM : Memory::RCP<RDRAM> {
  Node::Object node;

  struct Chip {
    n1  present;
    n1  enable;
    n1  autoCurrent;
    n16 deviceID;
    n3  writeDelay;
    n6  cci;
    n6  ccInternal;
    n6  ccLow;
    n6  ccHigh;
    n32 deviceType;
    n32 deviceIDReg;
    n32 delay;
    n32 mode;
    n32 refreshInterval;
    n32 refreshRow;
    n32 rasInterval;
    n32 minInterval;
    n32 addressSelect;
    n32 deviceManufacturer;
    n32 currentControl;
    n32 row;
  };

  struct Writable : public Memory::Writable {
    RDRAM& self;

    Writable(RDRAM& self) : self(self) {}

    auto translate(u32 address) -> maybe<u32>;
    auto degrade(u32 address, u64 value, u32 chipIndex) -> u64;

    template<u32 Size>
    auto read(u32 address, RBusDevice device) -> u64 {
      if(unlikely(!self.mapIdentity)) {
        auto mapped = translate(address);
        if(!mapped) {
          if(device < RBusDevice::NUM_RBUS_HW_DEVICES) ri.ackError();
          return 0;
        }
        u32 chipIndex = mapped() / 2_MiB;
        u64 value = Memory::Writable::read<Size>(mapped());
        return degrade(mapped(), value, chipIndex);
      }
      if(address >= size) return 0;
      if(unlikely(system.homebrewMode)) {
        self.debugger.readWord(address, Size, device);
        self.profile.metrics[(u32)device].reads += Size;
      }
      return Memory::Writable::read<Size>(address);
    }

    template<u32 Size>
    auto writeRepeat(u32 address, u64 value, u8 length) -> void {
      if constexpr(Size == Byte) {
        value = value & (0xFFFFFFFF >> (24 - (address & 3) * 8));
        value = (u32)((value << 24) | (value >> 8));
      } else if constexpr(Size == Half) {
        value = value & (0xFFFFFFFF >> (16 - (address & 2) * 8));
        value = (u32)((value << 16) | (value >> 16));
      }
      if constexpr(Size != Dual)
        value = (value << 32) | (u32) value;

      const u32 end = min((address & ~7) + length, size);
      if(end <= address) return;

      length = end - address;

      if(address & 1) {
        Memory::Writable::write<Byte>(address, value >> 56);
        value = (value << 8) | (value >> 56);
        address = (address & ~0x7FF) | ((address + 1) & 0x7FF);
        length -= 1;
      }
      if((address & 2) && length >= 2) {
        Memory::Writable::write<Half>(address, value >> 48);
        value = (value << 16) | (value >> 48);
        address = (address & ~0x7FF) | ((address + 2) & 0x7FF);
        length -= 2;
      }
      if((address & 4) && length >= 4) {
        Memory::Writable::write<Word>(address, value >> 32);
        value = (value << 32) | (value >> 32);
        address = (address & ~0x7FF) | ((address + 4) & 0x7FF);
        length -= 4;
      }

      while(length >= 8) {
        Memory::Writable::write<Dual>(address, value);
        address = (address & ~0x7FF) | ((address + 8) & 0x7FF);
        length -= 8;
      }
      if(length >= 4) {
        Memory::Writable::write<Word>(address, value >> 32);
        value <<= 32;
        address += 4;
        length -= 4;
      }
      if(length >= 2) {
        Memory::Writable::write<Half>(address, value >> 48);
        value <<= 16;
        address += 2;
        length -= 2;
      }
      if(length == 1)
        Memory::Writable::write<Byte>(address, value >> 56);
    }

    template<u32 Size>
    auto write(u32 address, u64 value, RBusDevice device) -> void {
      if(unlikely(!self.mapIdentity)) {
        auto mapped = translate(address);
        if(!mapped) {
          if(device < RBusDevice::NUM_RBUS_HW_DEVICES) ri.ackError();
          mi.initializeMode();
          return;
        }
        address = mapped();
      } else {
        if(address >= size) return;
      }
      if(unlikely(system.homebrewMode)) {
        self.debugger.writeWord(address, Size, value, device);
      }
      if(unlikely(mi.initializeMode())) {
        u32 len = mi.initializeLength() + 1;
        writeRepeat<Size>(address, value, len);
        if(unlikely(system.homebrewMode)) {
          self.profile.metrics[(u32)device].writes += len;
        }
      } else {
        Memory::Writable::write<Size>(address, value);
        if(unlikely(system.homebrewMode)) {
          self.profile.metrics[(u32)device].writes += Size;
        }
      }
    }

    template<u32 Size>
    auto writeBurst(u32 address, u32 *value, RBusDevice device) -> void {
      if(unlikely(!self.mapIdentity)) {
        auto mapped = translate(address);
        if(!mapped) {
          if(device < RBusDevice::NUM_RBUS_HW_DEVICES) ri.ackError();
          return;
        }
        address = mapped();
      } else {
        if(address >= size) return;
      }
      if(unlikely(system.homebrewMode)) {
        self.profile.metrics[(u32)device].writes += Size;
      }
      Memory::Writable::write<Word>(address | 0x00, value[0]);
      Memory::Writable::write<Word>(address | 0x04, value[1]);
      Memory::Writable::write<Word>(address | 0x08, value[2]);
      Memory::Writable::write<Word>(address | 0x0c, value[3]);
      if(Size == ICache) {
        Memory::Writable::write<Word>(address | 0x10, value[4]);
        Memory::Writable::write<Word>(address | 0x14, value[5]);
        Memory::Writable::write<Word>(address | 0x18, value[6]);
        Memory::Writable::write<Word>(address | 0x1c, value[7]);
      }
    }

    template<u32 Size>
    auto readBurst(u32 address, u32 *value, RBusDevice device) -> void {
      if(unlikely(!self.mapIdentity)) {
        auto mapped = translate(address);
        if(!mapped) {
          if(device < RBusDevice::NUM_RBUS_HW_DEVICES) ri.ackError();
          value[0] = value[1] = value[2] = value[3] = 0;
          if(Size == ICache) value[4] = value[5] = value[6] = value[7] = 0;
          return;
        }
        u32 chipIndex = mapped() / 2_MiB;
        address = mapped();
        value[0] = degrade(address | 0x00, Memory::Writable::read<Word>(address | 0x00), chipIndex);
        value[1] = degrade(address | 0x04, Memory::Writable::read<Word>(address | 0x04), chipIndex);
        value[2] = degrade(address | 0x08, Memory::Writable::read<Word>(address | 0x08), chipIndex);
        value[3] = degrade(address | 0x0c, Memory::Writable::read<Word>(address | 0x0c), chipIndex);
        if(Size == ICache) {
          value[4] = degrade(address | 0x10, Memory::Writable::read<Word>(address | 0x10), chipIndex);
          value[5] = degrade(address | 0x14, Memory::Writable::read<Word>(address | 0x14), chipIndex);
          value[6] = degrade(address | 0x18, Memory::Writable::read<Word>(address | 0x18), chipIndex);
          value[7] = degrade(address | 0x1c, Memory::Writable::read<Word>(address | 0x1c), chipIndex);
        }
        return;
      }
      if(address >= size) {
        value[0] = value[1] = value[2] = value[3] = 0;
        if(Size == ICache) value[4] = value[5] = value[6] = value[7] = 0;
        return;
      }
      if(unlikely(system.homebrewMode)) {
        self.profile.metrics[(u32)device].reads += Size;
      }
      value[0] = Memory::Writable::read<Word>(address | 0x00);
      value[1] = Memory::Writable::read<Word>(address | 0x04);
      value[2] = Memory::Writable::read<Word>(address | 0x08);
      value[3] = Memory::Writable::read<Word>(address | 0x0c);
      if(Size == ICache) {
        value[4] = Memory::Writable::read<Word>(address | 0x10);
        value[5] = Memory::Writable::read<Word>(address | 0x14);
        value[6] = Memory::Writable::read<Word>(address | 0x18);
        value[7] = Memory::Writable::read<Word>(address | 0x1c);
      }
    }

  } ram{*this};

  struct Debugger {
    u32 lastReadCacheline    = 0xffff'ffff;
    u32 lastWrittenCacheline = 0xffff'ffff;

    //debugger.cpp
    auto load(Node::Object) -> void;
    auto io(bool mode, u32 chipID, u32 address, u32 data) -> void;
    auto readWord(u32 address, int size, RBusDevice device) -> void;
    auto writeWord(u32 address, int size, u64 value, RBusDevice device) -> void;
    auto cacheErrorContext(string device) -> string;

    struct Memory {
      Node::Debugger::Memory ram;
      Node::Debugger::Memory dcache;
    } memory;

    struct Tracer {
      Node::Debugger::Tracer::Notification io;
    } tracer;
  } debugger;

  //rdram.cpp
  auto requestorName(RBusDevice requestor) -> const char* {
    switch(requestor) {
      case RBusDevice::VR4300_ICACHE:   return "VR4300 ICache";
      case RBusDevice::VR4300_DCACHE:   return "VR4300 DCache";
      case RBusDevice::VR4300_UNCACHED: return "VR4300 Uncached";
      case RBusDevice::SP_DMA:          return "SP DMA";
      case RBusDevice::PI_DMA:          return "PI DMA";
      case RBusDevice::SI_DMA:          return "SI DMA";
      case RBusDevice::VI_DMA:          return "VI DMA";
      case RBusDevice::AI_DMA:          return "AI DMA";
      case RBusDevice::DP_DMA:          return "DP DMA";
      case RBusDevice::DP_DRAW:         return "DP Draw";
      case RBusDevice::ARES_DEBUGGER:   return "Ares Debugger";
      case RBusDevice::ARES_JIT:        return "Ares JIT";
      case RBusDevice::ARES_FLASH:      return "Ares Flash";
      default:                          return "Unknown";
    }
  }

  struct Metric {
    u64 reads, writes;
    auto total() const -> u64 { return reads + writes; }
  };

  struct Profile {
    Metric metrics[(u32)RBusDevice::NUM_RBUS_DEVICES];

    auto total() -> Metric {
      Metric total;
      for(u32 n = 0; n < (u32)RBusDevice::NUM_RBUS_HW_DEVICES; n++) {
        total.reads  += metrics[n].reads;
        total.writes += metrics[n].writes;
      }
      return total;
    }
  } profile;

  auto load(Node::Object) -> void;
  auto unload() -> void;
  auto power(bool reset) -> void;
  auto updateMapping() -> void;
  auto decodeDeviceID(u32 value) -> u16;
  auto decodeCCI(u32 mode) -> n6;
  auto encodeCCI(n6 cci) -> u32;
  auto selectChip(u32 select, bool broadcast) -> Chip*;
  auto chipIndex(Chip* chip) -> u32;
  auto readRegister(Chip& chip, u32 index) -> u32;
  auto writeRegister(Chip& chip, u32 index, u32 data, bool repeat) -> void;

  //io.cpp
  auto readWord(u32 address, Thread& thread) -> u32;
  auto writeWord(u32 address, u32 data, Thread& thread) -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  Chip chips[4];
  n1 mapIdentity = 0;
};

extern RDRAM rdram;
