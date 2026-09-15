struct System {
  Node::System node;
  Node::Setting::Boolean fastBoot;
  VFS::Pak pak;
  bool homebrewMode = false;

  enum class Region : u32 { NTSCJ, NTSCU, PAL };

  struct Clock {
    static constexpr u32 CPUCompatibility = 33'868'500;
    static constexpr u32 CPUNominal = 33'868'800;
    static constexpr u32 GPUNtsc = 53'693'175;
    static constexpr u32 GPUPal = 53'203'425;
  };

  auto name() const -> string { return information.name; }
  auto region() const -> Region { return information.region; }
  auto frequency() const -> u32 { return information.timing.cpuFrequency; }
  auto gpuFrequency() const -> u32 { return information.timing.gpuFrequency; }

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
    string name = "PlayStation";
    Region region = Region::NTSCJ;
    struct Timing {
      u32 cpuFrequency = Clock::CPUCompatibility;
      u32 gpuFrequency = Clock::GPUNtsc;
    } timing;
  } information;

  //serialization.cpp
  auto serialize(serializer&, bool synchronize) -> void;
};

extern Random random;
extern System system;

auto Region::NTSCJ() -> bool { return system.region() == System::Region::NTSCJ; }
auto Region::NTSCU() -> bool { return system.region() == System::Region::NTSCU; }
auto Region::PAL()   -> bool { return system.region() == System::Region::PAL;   }
