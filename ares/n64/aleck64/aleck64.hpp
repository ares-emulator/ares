struct Aleck64 {
  Node::Object dipSwitchNode;
  enum BoardType { E90, E92 };

  struct Writable : public Memory::Writable {
    Aleck64& self;

    Writable(Aleck64& self) : self(self) {}

    template<u32 Size>
    auto writeBurst(u32 address, u32 *value, const char *peripheral) -> void {
      address = address & 0x00ff'ffff;
      if(address >= size) return;
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
    auto readBurst(u32 address, u32 *value, const char *peripheral) -> void {
      address = address & 0x00ff'ffff;
      if(address >= size) {
        value[0] = value[1] = value[2] = value[3] = 0;
        if(Size == ICache)
          value[4] = value[5] = value[6] = value[7] = 0;
        return;
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

  } sdram{*this};

  Memory::Writable vram;
  Memory::Writable pram;

  struct Debugger {
    Node::Object parent;
    //debugger.cpp
    auto load(Node::Object) -> void;
    auto unload() -> void;

    struct Memory {
      Node::Debugger::Memory sdram;
      Node::Debugger::Memory vram;
      Node::Debugger::Memory pram;
    } memory;
  } debugger;

  auto load(Node::Object) -> void;
  auto unload() -> void;
  auto power(bool reset) -> void;
  auto save() -> void;

  template<u32 Size>
  auto formatRead(u32 address, u32 data) -> u32 {
    if constexpr(Size == Byte) {;
      switch(address & 3) {
        case 0: return data >> 24;
        case 1: return data >> 16;
        case 2: return data >>  8;
        case 3: return data >>  0;
      }
    }
    if constexpr(Size == Half) {
      switch(address & 2) {
        case 0: return data >> 16;
        case 2: return data >>  0;
      }
    }
    if constexpr(Size == Word) {
      return data;
    }

    unreachable;
  }

  template<u32 Size> auto read(u32 address, Thread& thread) -> u32 {
    if(address <= 0xc07f'ffff) {
      return sdram.read<Size>(address & 0x00ff'ffff);
    }

    controls.poll();
    if(address <= 0xc080'0fff) {
      n32 value = 0xffff'ffff;

      switch (address & 0xffff'fffc) {
        case 0xc080'0000: return formatRead<Size>(address, readPort1());
        case 0xc080'0004: return formatRead<Size>(address, readPort2());
        case 0xc080'0008: return formatRead<Size>(address, readPort3());
        case 0xc080'0100: return formatRead<Size>(address, readPort4());
      }
    }

    if(gameConfig->type() == BoardType::E90) {
      if(address >= 0xd000'0000 && address <= 0xd000'0fff) return vram.read<Size>(address & 0x0000'0fff);
      if(address >= 0xd001'0000 && address <= 0xd001'0fff) return pram.read<Size>(address & 0x0000'0fff);
      if(address >= 0xd003'0000 && address <= 0xd003'001f) return formatRead<Size>(address, vdp.readWord(address & 0x0000'001f));
    } else if(gameConfig->type() == BoardType::E92) {
      if(address >= 0xd080'0000 && address <= 0xd080'0fff) return vram.read<Size>(address & 0x0000'0fff);
      if(address >= 0xd080'1000 && address <= 0xd080'1fff) return pram.read<Size>(address & 0x0000'0fff);
      if(address >= 0xd080'2000 && address <= 0xd080'201f) return formatRead<Size>(address, vdp.readWord(address & 0x0000'001f));
    }

    debug(unusual, "[Aleck64::read] Unmapped address: 0x", hex(address, 8L));
    return 0xffffffff;
  }

  template<u32 Size> auto write(u32 address, u32 data, Thread& thread) -> void {
    if(address <= 0xc07f'ffff) {
      return sdram.write<Size>(address & 0x00ff'ffff, data);
    }

    if(address <= 0xc080'0fff) {
      switch (address & 0xffff'fffc) {
        case 0xc080'0008: return writePort3(data);
        case 0xc080'0100: return writePort4(data);
      }
    }

    if(gameConfig->type() == BoardType::E90) {
      if(address >= 0xd000'0000 && address <= 0xd000'0fff) return vram.write<Size>(address & 0x0000'0fff, data);
      if(address >= 0xd001'0000 && address <= 0xd001'0fff) return pram.write<Size>(address & 0x0000'0fff, data);
      if(address >= 0xd003'0000 && address <= 0xd003'001f) return vdp.writeWord   (address & 0x0000'001f, data);
    } else if(gameConfig->type() == BoardType::E92) {
      if(address >= 0xd080'0000 && address <= 0xd080'0fff) return vram.write<Size>(address & 0x0000'0fff, data);
      if(address >= 0xd080'1000 && address <= 0xd080'1fff) return pram.write<Size>(address & 0x0000'0fff, data);
      if(address >= 0xd080'2000 && address <= 0xd080'201f) return vdp.writeWord   (address & 0x0000'001f, data);
    }

    debug(unusual, "[Aleck64::write] ", hex(address, 8L), " = ", hex(data, 8L));
  }

  //io.cpp
  auto readPort1() -> u32;
  auto readPort2() -> u32;
  auto readPort3() -> u32;
  auto readPort4() -> u32;
  auto writePort3(n32 data) -> void;
  auto writePort4(n32 data) -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  struct Controls {
    Aleck64& self;
    Node::Object node;

    Node::Input::Button service;
    Node::Input::Button test;

    Node::Input::Axis p1x;
    Node::Input::Axis p1y;
    Node::Input::Button p1up;
    Node::Input::Button p1down;
    Node::Input::Button p1left;
    Node::Input::Button p1right;
    Node::Input::Button p1[9];
    Node::Input::Button p1start;
    Node::Input::Button p1coin;

    Node::Input::Axis p2x;
    Node::Input::Axis p2y;
    Node::Input::Button p2up;
    Node::Input::Button p2down;
    Node::Input::Button p2left;
    Node::Input::Button p2right;
    Node::Input::Button p2[9];
    Node::Input::Button p2start;
    Node::Input::Button p2coin;

    Node::Input::Button mahjongA;
    Node::Input::Button mahjongB;
    Node::Input::Button mahjongC;
    Node::Input::Button mahjongD;
    Node::Input::Button mahjongE;
    Node::Input::Button mahjongF;
    Node::Input::Button mahjongG;
    Node::Input::Button mahjongH;
    Node::Input::Button mahjongI;
    Node::Input::Button mahjongJ;
    Node::Input::Button mahjongK;
    Node::Input::Button mahjongL;
    Node::Input::Button mahjongM;
    Node::Input::Button mahjongN;
    Node::Input::Button mahjongKan;
    Node::Input::Button mahjongPon;
    Node::Input::Button mahjongChi;
    Node::Input::Button mahjongReach;
    Node::Input::Button mahjongRon;

    //controls.cpp
    auto load(Node::Object) -> void;
    auto poll() -> void;

    auto controllerButton(int playerIndex, string button) -> bool;
    auto controllerAxis(int playerIndex, string axis) -> s64;
    auto mahjong(n8 row) -> n8;

  } controls{*this};

  struct VDP {
    auto readWord(u32 address) -> u32;
    auto writeWord(u32 address, n32 data) -> void;
    auto render(Node::Video::Screen screen) -> void;

    struct IO {
      n1 enable;
    } io;
  } vdp;

  n8 dipSwitch[2];

  struct GameConfig {
    virtual ~GameConfig() = default;
    virtual auto type() -> BoardType { return BoardType::E92; }
    virtual auto dpadDisabled() -> n1 { return 0; }
    virtual auto dipSwitches(Node::Object parent) -> void {};
    virtual auto readExpansionPort() -> u32 { return 0xffff'ffff; }
    virtual auto writeExpansionPort(n32 data) -> void {};
  };

  std::shared_ptr<GameConfig> gameConfig;
};

extern Aleck64 aleck64;
