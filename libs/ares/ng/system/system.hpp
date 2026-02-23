struct System {
  Node::System node;
  VFS::Pak pak;
  Memory::Readable<n16> bios;
  Memory::Readable<n16> srom;
  Memory::Writable<n16> wram;
  Memory::Writable<n16> sram;  //MVS only

  struct Debugger {
    //debugger.cpp
    auto load(Node::Object) -> void;
    auto unload(Node::Object) -> void;

    struct Memory {
      Node::Debugger::Memory bios;
      Node::Debugger::Memory srom;
      Node::Debugger::Memory wram;
      Node::Debugger::Memory sram;
    } memory;
  } debugger;

  enum class Model : u32 { NeoGeoAES, NeoGeoMVS };

  auto name() const -> string { return information.name; }
  auto model() const -> Model { return information.model; }

  //system.cpp
  auto game() -> string;
  auto run() -> void;

  auto load(Node::System& node, string name) -> bool;
  auto unload() -> void;
  auto save() -> void;
  auto power(bool reset = false) -> void;

  //serialization.cpp
  auto serialize(bool synchronize) -> serializer;
  auto unserialize(serializer&) -> bool;

  struct IO {
    n1 sramLock = 1;
    n3 slotSelect;
    n1 ledMarquee;
    n1 ledLatch1;
    n1 ledLatch2;
    n8 ledData;
  } io;

private:
  struct Information {
    string name;
    Model model = Model::NeoGeoAES;
  } information;

  //serialization.cpp
  auto serialize(serializer&, bool synchronize) -> void;
};

extern System system;

auto Model::NeoGeoAES() -> bool { return system.model() == System::Model::NeoGeoAES; }
auto Model::NeoGeoMVS() -> bool { return system.model() == System::Model::NeoGeoMVS; }
