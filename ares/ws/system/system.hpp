struct System : IO {
  Node::System node;
  Node::Setting::Boolean headphones;
  VFS::Pak pak;

  enum class SoC : u32 {
    ASWAN,
    SPHINX,
    SPHINX2,
  };

  enum class Model : u32 {
    WonderSwan,
    WonderSwanColor,
    SwanCrystal,
    PocketChallengeV2,
  };

  struct Controls {
    Node::Object node;

    //WonderSwan, WonderSwan Color, SwanCrystal
    Node::Input::Button y1;
    Node::Input::Button y2;
    Node::Input::Button y3;
    Node::Input::Button y4;
    Node::Input::Button x1;
    Node::Input::Button x2;
    Node::Input::Button x3;
    Node::Input::Button x4;
    Node::Input::Button b;
    Node::Input::Button a;
    Node::Input::Button start;
    Node::Input::Button volume;

    //Pocket Challenge V2
    Node::Input::Button up;
    Node::Input::Button down;
    Node::Input::Button left;
    Node::Input::Button right;
    Node::Input::Button pass;
    Node::Input::Button circle;
    Node::Input::Button clear;
    Node::Input::Button view;
    Node::Input::Button escape;

    //all models
    Node::Input::Button power;

    //controls.cpp
    auto load(Node::Object) -> void;
    auto poll() -> void;

    bool xHold = 0;
    bool leftLatch = 0;
    bool rightLatch = 0;
  } controls;

  struct Debugger {
    System& self;

    //debugger.cpp
    auto load(Node::Object) -> void;
    auto unload(Node::Object) -> void;

    struct Memory {
      Node::Debugger::Memory eeprom;
    } memory;
  } debugger{*this};

  auto name() const -> string { return information.name; }
  auto model() const -> Model { return information.model; }
  auto soc() const -> SoC { return information.soc; }
  auto mode() const -> n3 { return io.mode; }
  auto memory() const -> u32 { return io.mode.bit(2) == 0 ? 16_KiB : 64_KiB; }

  //mode:
  //xx0 => planar tiledata
  //xx1 => packed tiledata
  //x0x => 512 tiles
  //x1x => 1024 tiles
  //0xx => 16 KiB memory mode
  //1xx => 64 KiB memory mode
  //00x => 2bpp, grayscale
  //01x => 2bpp, color
  //10x => 2bpp, color
  //11x => 4bpp, color

  //system.cpp
  auto game() -> string;
  auto run() -> void;

  auto load(Node::System& node, string name) -> bool;
  auto unload() -> void;
  auto save() -> void;
  auto power(bool reset = false) -> void;

  //io.cpp
  auto readIO(n16 address) -> n8 override;
  auto writeIO(n16 address, n8 data) -> void override;

  //serialization.cpp
  auto serialize(bool synchronize) -> serializer;
  auto unserialize(serializer&) -> bool;

  struct Information {
    string name = "WonderSwan";
    SoC soc = SoC::ASWAN;
    Model model = Model::WonderSwan;
  } information;

  Memory::Readable<n8> bootROM;
  EEPROM eeprom;

private:
  struct IO {
    //$0060  DISP_MODE
    n1 unknown0;
    n1 unknown1;
    n1 unknown3;
    n3 mode;
  } io;

  //serialization.cpp
  auto serialize(serializer&, bool synchronize) -> void;
};

extern System system;

auto SoC::ASWAN() -> bool { return system.soc() == System::SoC::ASWAN; }
auto SoC::SPHINX() -> bool { return system.soc() == System::SoC::SPHINX; }
auto SoC::SPHINX2() -> bool { return system.soc() == System::SoC::SPHINX2; }

auto Model::WonderSwan() -> bool { return system.model() == System::Model::WonderSwan; }
auto Model::WonderSwanColor() -> bool { return system.model() == System::Model::WonderSwanColor; }
auto Model::SwanCrystal() -> bool { return system.model() == System::Model::SwanCrystal; }
auto Model::PocketChallengeV2() -> bool { return system.model() == System::Model::PocketChallengeV2; }
