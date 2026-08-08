struct SC64 : PIDevice {
  file_buffer image;
  Memory::Writable sdram;
  Memory::Writable buffer;

  // BlockRAM sections
  static constexpr u32 DataBufferSize      = 0x2000;
  static constexpr u32 EepromOffset        = 0x2000;
  static constexpr u32 McuBufferOffset     = 0x2800;
  static constexpr u32 FlashBufferOffset   = 0x2c00;
  static constexpr u32 BlockRamSize        = 0x2c80;
  static constexpr u32 FlashEraseBlockSize = 128 * 1024;

  // Address windows on the PI bus, and the bases for the same regions in
  // the host address space.
  static constexpr u32 SdramSize        = 0x0400'0000;
  static constexpr u32 PiSdramBase      = 0x1000'0000;
  static constexpr u32 PiBlockRamBase   = 0x1ffe'0000;
  static constexpr u32 PiRegisterBase   = 0x1fff'0000;
  static constexpr u32 RegisterAreaSize = 0x1c;
  static constexpr u32 HostBlockRamBase = 0x0500'0000;

  static constexpr u32 SectorSize     = 512;
  static constexpr u32 SdInfoSize     = 32;           // CSD + CID
  static constexpr u32 MaxSectorCount = 0x007f'ffff;  // count * SectorSize must fit in 32 bits
  static constexpr u32 UsbLengthMask  = 0x00ff'ffff;  // 24-bit length field
  static constexpr u32 MaxHostPayload = 16 * 1024 * 1024;

  static constexpr u64 UsbInputTimeout   = 1'000'000;  // microseconds
  static constexpr u64 KeepAliveInterval = 5'000'000;  // microseconds

  static constexpr u32 FirmwareVersion  = (2 << 16) | 20;  // v2.20
  static constexpr u32 FirmwareRevision = 2;

  // SCR bits. Each IRQ pending bit is one position above its mask (enable) bit.
  struct Scr {
    static constexpr u32 CmdBusy       = 1u << 31;
    static constexpr u32 CmdError      = 1u << 30;
    static constexpr u32 BtnIrqPending = 1u << 29;
    static constexpr u32 BtnIrqMask    = 1u << 28;
    static constexpr u32 CmdIrqPending = 1u << 27;
    static constexpr u32 CmdIrqMask    = 1u << 26;
    static constexpr u32 UsbIrqPending = 1u << 25;
    static constexpr u32 UsbIrqMask    = 1u << 24;
    static constexpr u32 AuxIrqPending = 1u << 23;
    static constexpr u32 AuxIrqMask    = 1u << 22;
    static constexpr u32 CmdIrqRequest = 1u << 8;
    static constexpr u32 CommandMask   = 0xff;
  };

  // IRQ register write bits.
  struct Irq {
    static constexpr u32 BtnClear   = 1u << 31;
    static constexpr u32 CmdClear   = 1u << 30;
    static constexpr u32 UsbClear   = 1u << 29;
    static constexpr u32 AuxClear   = 1u << 28;
    static constexpr u32 UsbDisable = 1u << 11;
    static constexpr u32 UsbEnable  = 1u << 10;
    static constexpr u32 AuxDisable = 1u << 9;
    static constexpr u32 AuxEnable  = 1u << 8;
  };

  // Key register values: the unlock sequence and the lock command.
  static constexpr u32 KeyReset   = 0x0000'0000;
  static constexpr u32 KeyUnlock1 = 0x5f55'4e4c;  // "_UNL"
  static constexpr u32 KeyUnlock2 = 0x4f43'4b5f;  // "OCK_"
  static constexpr u32 KeyLock    = 0xffff'ffff;

  // Register window at PiRegisterBase.
  enum class Register : u32 {
    Scr        = 0x00,
    Data0      = 0x04,
    Data1      = 0x08,
    Identifier = 0x0c,
    Key        = 0x10,
    Irq        = 0x14,
    Aux        = 0x18,
  };

  // IS64 protocol: a ring buffer in SDRAM that starts with a token.
  static constexpr u32 IsvToken        = 0x4953'3634;  // "IS64"
  static constexpr u32 IsvReadPointer  = 0x04;
  static constexpr u32 IsvWritePointer = 0x14;
  static constexpr u32 IsvBufferOffset = 0x20;
  static constexpr u32 IsvWindowSize   = 64 * 1024;
  static constexpr u32 IsvSetupToken   = 0x100;
  static constexpr u32 IsvSetupAddress = 0x104;
  static constexpr u32 IsvSetupReady   = 0x10c;

  // Frame types of the remote transport protocol.
  enum class RemoteType : u32 {
    Command = 1,
    Response = 2,
    Packet = 3,
    KeepAlive = 0xcafe'beef,
  };

  // Identifiers for packets that go to the USB host.
  enum class PacketId : u8 {
    ButtonTrigger = 'B',
    DataFlushed   = 'G',
    IsvOutput     = 'I',
    DebugOutput   = 'U',
    AuxData       = 'X',
  };

  struct Packet {
    u8 type = 0;
    std::vector<u8> data;
  };

  struct Host : nall::TCP::Socket {
    SC64& self;

    Host(SC64& self) : self(self) {}

    auto poll() -> void { update(); }
    auto send(const std::vector<u8>& data) -> void { sendData(data.data(), data.size()); }
    auto sendPacket(PacketId id, const std::vector<u8>& data) -> void;

  protected:
    auto onData(const std::vector<u8>& data) -> void override;
    auto onConnect() -> void override;
    auto onDisconnect() -> void override;
  } host;

  struct Registers {
    u32 scr = 0;
    u32 data0 = 0;
    u32 data1 = 0;
    u32 identifier = 0x53437632;  // "SCv2"
    u32 aux = 0;
  } registers;

  enum class HostMode : u8 { Unknown, Direct, Remote };

  // Commands that the N64 sends through the register interface.
  enum class Command : u8 {
    IdentifierGet    = 'v',
    VersionGet       = 'V',
    ConfigGet        = 'c',
    ConfigSet        = 'C',
    SettingGet       = 'a',
    SettingSet       = 'A',
    TimeGet          = 't',
    TimeSet          = 'T',
    UsbRead          = 'm',
    UsbWrite         = 'M',
    UsbReadStatus    = 'u',
    UsbWriteStatus   = 'U',
    SdCardOp         = 'i',
    SdSectorSet      = 'I',
    SdRead           = 's',
    SdWrite          = 'S',
    DiskMappingSet   = 'D',
    WritebackPending = 'w',
    WritebackSdInfo  = 'W',
    FlashProgram     = 'K',
    FlashWaitBusy    = 'p',
    FlashEraseBlock  = 'P',
    DiagnosticGet    = '%',
  };

  // Commands that the USB host sends.
  // Some letters have a different meaning on each side.
  enum class HostCommand : u8 {
    IdentifierGet   = 'v',
    VersionGet      = 'V',
    StateReset      = 'R',
    CicParamsSet    = 'B',
    ConfigGet       = 'c',
    ConfigSet       = 'C',
    SettingGet      = 'a',
    SettingSet      = 'A',
    TimeGet         = 't',
    TimeSet         = 'T',
    MemoryRead      = 'm',
    MemoryWrite     = 'M',
    UsbWrite        = 'U',
    AuxWrite        = 'X',
    SdCardOp        = 'i',
    SdRead          = 's',
    SdWrite         = 'S',
    DdSetBlockReady = 'D',
    WritebackEnable = 'W',
    FlashWaitBusy   = 'p',
    FlashEraseBlock = 'P',
    UpdateBackup    = 'f',
    UpdatePrepare   = 'F',
    DebugGet        = '?',
    DiagnosticGet   = '%',
  };

  enum class Config : u32 {
    BootloaderSwitch,
    RomWriteEnable,
    RomShadowEnable,
    DdMode,
    IsvAddress,
    BootMode,
    SaveType,
    CicSeed,
    TvType,
    DdSdEnable,
    DdDriveType,
    DdDiskState,
    ButtonState,
    ButtonMode,
    RomExtendedEnable,
  };

  // Config::SaveType values (SC64 firmware save_type_t).
  enum class SaveType : u32 {
    None = 0,
    Eeprom4k = 1,
    Eeprom16k = 2,
    Sram = 3,
    Flashram = 4,
    SramBanked = 5,
    Sram1M = 6,
  };

  // Sub-operations of the SdCardOp command.
  // Both sides use the same set.
  enum class SdOp : u32 {
    Deinit,
    Init,
    GetStatus,
    GetInfo,
    ByteSwapOn,
    ByteSwapOff,
  };

  // When a command fails, DATA0 holds the error type and an error code.
  enum class ErrorType : u32 { Cfg = 1, Sd = 2 };
  enum class CfgError : u32 {
    UnknownCommand  = 1,
    InvalidArgument = 2,
    InvalidAddress  = 3,
    InvalidId       = 4,
  };
  enum class SdError : u32 {
    Ok               = 0,
    NoCardInSlot     = 1,
    NotInitialized   = 2,
    InvalidArgument  = 3,
    InvalidAddress   = 4,
    InvalidOperation = 5,
    Cmd25Io          = 23,
    Locked           = 30,
  };
  enum class SdLock : u32 { None, N64, USB };

  enum class Target : u32 { None, Buffer, Sdram, Registers };
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
  u64 hostLastKeepAlive = 0;

  SC64() : host(*this) {}

  auto open(string location, bool readOnly, u32 hostPort = 0) -> void;
  auto close() -> void;
  auto power(bool reset) -> void;
  auto flush() -> void;
  auto pollHost() -> void;
  auto config(Config id) -> u32& { return configs[(u32)id]; }
  auto config(Config id) const -> u32 { return configs[(u32)id]; }
  auto romWriteEnabled() const -> bool { return config(Config::RomWriteEnable); }
  auto irqLine() const -> bool;

  auto piAddress(u32 address, PIDeviceTiming timing) -> bool override;
  auto piReadHalf(PIDeviceTiming timing) -> maybe<u16> override;
  auto piWriteHalf(u16 data, PIDeviceTiming timing) -> void override;
  auto serialize(serializer& s) -> void;

private:
  auto resetConfigs() -> void;
  auto resetConfigState() -> void;
  auto pollIsViewer() -> void;
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
  auto readPiMemory(u32 address, u8* data, u32 length) -> bool;
  auto writePiMemory(u32 address, const u8* data, u32 length) -> bool;
  auto hostTransfer(bool write, u32 address, u32 sector, u32 count) -> SdError;
  auto sdTryLock(SdLock lock) -> SdError;
  auto sdGetLock(SdLock lock) const -> SdError;
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
  auto error(CfgError code) -> void { error(ErrorType::Cfg, (u32)code); }
  auto error(SdError code) -> void { error(ErrorType::Sd, (u32)code); }
  auto updateInterrupt() -> void;
  auto transfer(bool write, u32 piAddress, u32 count) -> SdError;
  auto setByteSwap(u8* data, u32 size) -> void;
};

extern SC64 sc64;
