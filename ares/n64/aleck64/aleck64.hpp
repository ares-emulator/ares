struct Aleck64 : Memory::RCP<Aleck64> {
  Node::Object dipSwitchNode;

  struct Writable : public Memory::Writable {
    Aleck64& self;

    Writable(Aleck64& self) : self(self) {}

    template<u32 Size>
    auto writeBurst(u32 address, u32 *value, const char *peripheral) -> void {
      address = address & 0x00ff'ffff;
      if (address >= size) return;
      Memory::Writable::write<Word>(address | 0x00, value[0]);
      Memory::Writable::write<Word>(address | 0x04, value[1]);
      Memory::Writable::write<Word>(address | 0x08, value[2]);
      Memory::Writable::write<Word>(address | 0x0c, value[3]);
      if (Size == ICache) {
        Memory::Writable::write<Word>(address | 0x10, value[4]);
        Memory::Writable::write<Word>(address | 0x14, value[5]);
        Memory::Writable::write<Word>(address | 0x18, value[6]);
        Memory::Writable::write<Word>(address | 0x1c, value[7]);
      }
    }

    template<u32 Size>
    auto readBurst(u32 address, u32 *value, const char *peripheral) -> void {
      address = address & 0x00ff'ffff
      if (address >= size) {
        value[0] = value[1] = value[2] = value[3] = 0;
        if (Size == ICache)
          value[4] = value[5] = value[6] = value[7] = 0;
        return;
      }
      value[0] = Memory::Writable::read<Word>(address | 0x00);
      value[1] = Memory::Writable::read<Word>(address | 0x04);
      value[2] = Memory::Writable::read<Word>(address | 0x08);
      value[3] = Memory::Writable::read<Word>(address | 0x0c);
      if (Size == ICache) {
        value[4] = Memory::Writable::read<Word>(address | 0x10);
        value[5] = Memory::Writable::read<Word>(address | 0x14);
        value[6] = Memory::Writable::read<Word>(address | 0x18);
        value[7] = Memory::Writable::read<Word>(address | 0x1c);
      }
    }

  } sdram{*this};
  auto load(Node::Object) -> void;
  auto unload() -> void;
  auto power(bool reset) -> void;
  auto save() -> void;
  auto applyGameSpecificSettings(string name) -> void;

  //io.cpp
  auto readWord(u32 address, Thread& thread) -> u32;
  auto writeWord(u32 address, u32 data, Thread& thread) -> void;

  auto readPort1() -> u32;
  auto readPort2() -> u32;
  auto readPort3() -> u32;
  auto writePort3(n32 data) -> void;

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
    auto ioPortControls(int port) -> n32;
    auto mahjong(n8 row) -> n8;

  } controls{*this};

  n8 dipSwitch[2];

  struct GameConfig {
    virtual ~GameConfig() = default;
    virtual auto dipSwitches(Node::Object parent) -> void = 0;

    virtual auto controllerButton(int playerIndex, string button) -> bool { return 0; }
    virtual auto controllerAxis(int playerIndex, string axis) -> s64 { return 0; };
    virtual auto ioPortControls(int port) -> n32 { return 0xffff'ffff; }
    virtual auto readExpansionPort() -> u32 { return 0xffff'ffff; }
    virtual auto writeExpansionPort(n32 data) -> void {};
  };

  shared_pointer<GameConfig> gameConfig;
};

extern Aleck64 aleck64;
