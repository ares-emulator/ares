struct SC64 : PIDevice {
  Cartridge& self;
  file_buffer image;
  Memory::Writable sdram;
  Memory::Writable buffer;

  struct Packet {
    u8 type = 0;
    std::vector<u8> data;
  };

  struct Host : nall::TCP::Socket {
    SC64& self;

    Host(SC64& self) : self(self) {}

    auto poll() -> void { update(); }
    auto send(const std::vector<u8>& data) -> void { sendData(data.data(), data.size()); }
    auto sendPacket(u8 id, const std::vector<u8>& data) -> void;

  protected:
    auto onData(const std::vector<u8>& data) -> void override;
    auto onConnect() -> void override;
    auto onDisconnect() -> void override;
  } host;

  struct Registers {
    u32 scr = 0;
    u32 data0 = 0;
    u32 data1 = 0;
    u32 identifier = 0x53437632;
    u32 aux = 0;
  } registers;

  enum class HostMode : u8 { Unknown, Direct, Remote };

  // error_type_t (cfg.c:108-109): packed into DATA0's top byte on failure.
  enum class ErrorType : u32 { Cfg = 1, Sd = 2 };
  // sd_lock_t (sd.h:47-51): mutual exclusion between the N64 and USB-host
  // sides of the SD card interface.
  enum class SdLock : u32 { None, N64, USB };

  enum class Target : u32 { None, Buffer, USBMemory, Registers };
  Target target = Target::None;
  u32 offset = 0;
  u32 pendingRegister = 0;
  u32 keySequence = 0;
  u32 sdSector = 0;
  u64 sectorCount = 0;
  u32 hostPort = 0;
  std::vector<u8> hostInput;
  HostMode hostMode = HostMode::Unknown;
  std::deque<Packet> hostPendingPackets;
  std::deque<Packet> usbInput;
  u32 usbInputOffset = 0;
  u64 usbInputDeadline = 0;
  u32 configs[15] = {};
  u32 settings[1] = {};
  bool enabled = false;
  bool sdInserted = false;
  bool readOnly = false;
  bool unlocked = false;
  bool sdInitialized = false;
  bool sdBlockAddressed = false;
  bool sdClock50MHz = false;
  bool byteSwap = false;
  SdLock sdLock = SdLock::None;
  bool usbOutputBusy = false;
  bool hostConnected = false;
  u64 hostLastPoll = 0;
  u64 hostLastKeepAlive = 0;

  SC64(Cartridge& self) : self(self), host(*this) {}

  auto open(string location, bool readOnly, u32 hostPort = 0) -> bool;
  auto close() -> void;
  auto power(bool reset) -> void;
  auto flush() -> void;
  auto pollHost() -> void;
  auto romWriteEnabled() const -> bool { return configs[1]; }

  auto piAddress(u32 address, PIDeviceTiming timing) -> bool override;
  auto piReadHalf(PIDeviceTiming timing) -> maybe<u16> override;
  auto piWriteHalf(u16 data, PIDeviceTiming timing) -> void override;
  auto serialize(serializer& s) -> void;

private:
  auto resetConfigs() -> void;
  auto registerRead(u32 address) -> u32;
  auto registerWrite(u32 address, u32 data) -> void;
  auto hostData(const std::vector<u8>& data) -> void;
  auto hostDataDispatch() -> void;
  auto syncHostConnection() -> void;
  auto hostDataDirect() -> void;
  auto hostDataRemote() -> void;
  auto hostCommand(u8 command, u32 data0, u32 data1, const std::vector<u8>& data) -> void;
  auto hostResponse(u8 command, bool error, const std::vector<u8>& data) -> void;
  auto hostCommandDataLength(u8 command, u32 data1) const -> u32;
  auto hostConfigGet(u32 id, u32& value) const -> bool;
  auto hostConfigSet(u32 id, u32 value, u32& previous) -> bool;
  auto hostMemoryAddress(u32 address, u32 length) -> u8*;
  auto piMemoryAddress(u32 address, u32 length) -> u8*;
  auto piAddressValid(u32 address, u32 length) -> bool;
  auto readPiMemory(u32 address, u8* data, u32 length) -> bool;
  auto writePiMemory(u32 address, const u8* data, u32 length) -> bool;
  auto hostTransfer(bool write, u32 address, u32 sector, u32 count) -> u32;
  auto sdTryLock(SdLock lock) -> u32;
  auto sdGetLock(SdLock lock) const -> u32;
  auto sdReleaseLock(SdLock lock) -> void;
  auto resetUsb() -> void;
  auto enqueueUsbInput(u8 type, const std::vector<u8>& data) -> bool;
  auto usbReadStatus() const -> u32;
  auto sdInfo() const -> std::vector<u8>;
  auto readMemory(u32 address, u8* data, u32 size) -> bool;
  auto writeMemory(u32 address, const u8* data, u32 size) -> bool;
  auto usbWrite(u8 type, u32 address, u32 length) -> bool;
  auto usbRead(u32 address, u32 length) -> bool;
  auto sdStatus() const -> u32;
  auto writeSdInfo(u32 address) -> bool;
  auto execute(u8 command) -> void;
  auto complete() -> void;
  auto error(ErrorType type, u32 code) -> void;
  auto updateInterrupt() -> void;
  auto address(u32 value, u32 length) -> u8*;
  auto transfer(bool write, u32 piAddress, u32 count) -> u32;
  auto setByteSwap(u8* data, u32 size) -> void;
};
