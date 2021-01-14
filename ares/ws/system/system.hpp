struct System : IO {
  Node::System node;
  Node::Setting::Boolean headphones;

  enum class SoC : uint {
    ASWAN,
    SPHINX,
    SPHINX2,
  };

  enum class Model : uint {
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

  auto name() const -> string { return node->name(); }
  auto model() const -> Model { return information.model; }
  auto soc() const -> SoC { return information.soc; }
  auto mode() const -> uint3 { return io.mode; }
  auto memory() const -> uint { return io.mode.bit(2) == 0 ? 16_KiB : 64_KiB; }

  //mode:
  //xx0 => planar tiledata
  //xx1 => packed tiledata
  //x0x =>  512 tiles
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
  auto portRead(uint16 address) -> uint8 override;
  auto portWrite(uint16 address, uint8 data) -> void override;

  //serialization.cpp
  auto serialize(bool synchronize) -> serializer;
  auto unserialize(serializer&) -> bool;

  struct Information {
    SoC soc = SoC::ASWAN;
    Model model = Model::WonderSwan;
  } information;

  Memory::Readable<uint8> bootROM;
  EEPROM eeprom;

private:
  struct Registers {
    //$0060  DISP_MODE
    uint1 unknown0;
    uint1 unknown1;
    uint1 unknown3;
    uint3 mode;
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
