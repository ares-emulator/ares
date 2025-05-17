struct GS_EEPROM : Memory::Writable {
  Cartridge& self;
  GS_EEPROM(Cartridge& self) : self(self) {}

  template<u32 Size>
  auto read(u32 address) -> u64 {
    static_assert(Size == Byte);  //Only 8-bit accesses supported
    return readByte(address);
  }

  template<u32 Size>
  auto write(u32 address, u64 data) -> void {
    static_assert(Size == Byte);  //Only 8-bit accesses supported
    return writeByte(address, data);
  }

  auto get(n20 address) -> u8 {
    n19 real_addr = address >> 1;
    if(real_addr <= size - 1) return Memory::Writable::read<Byte>(real_addr);
    return 0;
  }

  auto set(n20 address, u8 data) -> void {
    n19 real_addr = address >> 1;
    if(real_addr <= size - 1) return Memory::Writable::write<Byte>(real_addr, data);
    return;
  }

  enum Type : u32 {
    SST_29LE010, //131072 cells => 256 x 8-bit x 512-page
    SST_29EE010,
    ATMEL_29LV010A, //131072 cells => 128 x 8-bit x 1024-page
    //SST_29LF040, //524288 cells => 256 x 8-bit x 2048-page
  };

  enum State : u32 {
    WaitStart,
    Expect55,
    ExpectCommand,
    WaitWrite,
    WaitSecondStart,
    ExpectSecond55,
    ExpectSecondCommand,
  };

  enum Mode : u32 {
    Normal,
    ReadID,
  };

  struct WriteBufferEntry {
    auto serialize(serializer& s) -> void { s(data); s(address); }

    u8 data;
    n19 address;
  };

  auto id() -> n16 {
    if(typeID == Type::ATMEL_29LV010A) return 0x1F35;
    //if(typeID == Type::SST_29LF040) return 0xBF04;
    if(typeID == Type::SST_29EE010) return 0xBF07;
    if(typeID == Type::SST_29LE010) return 0xBF08;
    return 0;
  }

  auto delay() -> u32 {
    if(typeID == Type::ATMEL_29LV010A) return 150;
    //if(typeID == Type::SST_29LF040) return 200;
    if(typeID == Type::SST_29EE010) return 200;
    if(typeID == Type::SST_29LE010) return 200;
    return 0;
  }

  //gs-eeprom.cpp
  auto readByte(n19 address) -> u8;
  auto writeByte(n19 address, u8 data) -> void;
  
  auto clock() -> void;

  auto serialize(serializer&) -> void;

  //TODO: some way to select the chip
  u32 typeID = Type::SST_29LE010;
  u32 state = State::WaitStart;
  u32 mode = Mode::Normal;

  bool sdp = true;
  u32 countdown = 0; //time until write cycle will start in microseconds
  WriteBufferEntry write_buffer[128];
  u32 entries = 0;
  n19 last_entry_address = 0;
};
