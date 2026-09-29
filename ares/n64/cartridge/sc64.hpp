struct SC64 : PIDevice {
  Cartridge& self;
  file_buffer image;
  Memory::Writable buffer;

  struct Registers {
    u32 scr = 0;
    u32 data0 = 0;
    u32 data1 = 0;
    u32 identifier = 0x53437632;
    u32 irq = 0;
    u32 aux = 0;
  } registers;

  enum class Target : u32 { None, Buffer, Registers };
  Target target = Target::None;
  u32 offset = 0;
  u32 pendingRegister = 0;
  u32 sdSector = 0;
  u64 sectorCount = 0;
  bool enabled = false;
  bool readOnly = false;
  bool unlocked = false;
  bool sdInitialized = false;
  bool sdBlockAddressed = false;
  bool sdClock50MHz = false;
  bool byteSwap = false;

  SC64(Cartridge& self) : self(self) {}

  auto open(string location, bool readOnly) -> bool;
  auto close() -> void;
  auto flush() -> void;

  auto piAddress(u32 address, PIDeviceTiming timing) -> bool override;
  auto piReadHalf(PIDeviceTiming timing) -> maybe<u16> override;
  auto piWriteHalf(u16 data, PIDeviceTiming timing) -> void override;

private:
  auto registerRead(u32 address) -> u32;
  auto registerWrite(u32 address, u32 data) -> void;
  auto sdStatus() const -> u32;
  auto writeSdInfo(u32 address) -> bool;
  auto execute(u8 command) -> void;
  auto complete() -> void;
  auto error() -> void;
  auto address(u32 value, u32 length) -> u8*;
  auto transfer(bool write, u32 piAddress, u32 count) -> bool;
  auto setByteSwap(u8* data, u32 size) -> void;
};
