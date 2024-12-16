extern Random random;

struct System {
  Node::System node;
  VFS::Pak pak;
  bool homebrewMode = false;
  bool expansionPak = true;
  u8 nand64[4] = { 0 };
  u8 nand128[4] = { 0 };
  bool is_128 = false;

  enum class Region : u32 { NTSC, PAL };

  auto name() const -> string { return information.name; }
  auto region() const -> Region { return information.region; }
  auto _DD() const -> bool { return information.dd; }
  auto _BB() const -> bool { return information.bb; }
  auto frequency() const -> u32 { return information.frequency; }
  auto videoFrequency() const -> u32 { return information.videoFrequency; }

  //system.cpp
  auto game() -> string;
  auto run() -> void;
  auto load(Node::System& node, string name) -> bool;
  auto unload() -> void;
  auto save() -> void;
  auto power(bool reset) -> void;

  //serialization.cpp
  auto serialize(bool synchronize = true) -> serializer;
  auto unserialize(serializer&) -> bool;

private:
  struct Information {
    string name = "Nintendo 64";
    Region region = Region::NTSC;
    u32 frequency = 93'750'000 * 2;
    u32 videoFrequency = 48'681'818;
    bool dd = false;
    bool bb = false;
  } information;

  auto initDebugHooks() -> void;

  //serialization.cpp
  auto serialize(serializer&, bool synchronize) -> void;
};

extern System system;

auto Region::NTSC() -> bool { return system.region() == System::Region::NTSC; }
auto Region::PAL() -> bool { return system.region() == System::Region::PAL; }
auto _DD() -> bool { return system._DD(); }
//auto _BB() -> bool { return system._BB(); }
//auto frequency() -> u32 { return system.frequency(); }