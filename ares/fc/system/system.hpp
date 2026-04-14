extern Random random;

struct System {
  Node::System node;

  struct Controls {
    Node::Object node;
    Node::Input::Button reset;
    Node::Input::Button microphone;

    //controls.cpp
    auto load(Node::Object) -> void;
    auto unload() -> void;
    auto poll() -> void;
  } controls;

  enum class Region : u32 { NTSCJ, NTSCU, PAL, Dendy };

  auto name()      const -> string { return information.name;      }
  auto region()    const -> Region { return information.region;    }
  auto frequency() const -> f64    { return information.frequency; }
  auto cpuDivider() const -> u8    { return information.cpuDivider; }

  //system.cpp
  auto game() -> string;
  auto run() -> void;

  auto load(Node::System& node, string name) -> bool;
  auto save() -> void;
  auto unload() -> void;
  auto power(bool reset) -> void;

  //serialization.cpp
  auto serialize(bool synchronize) -> serializer;
  auto unserialize(serializer&) -> bool;

private:
  struct Information {
    string name       = "Famicom";
    Region region     = Region::NTSCJ;
    f64    frequency  = Constants::Colorburst::NTSC * 6.0;
    u8     cpuDivider = 12;
  } information;

  //serialization.cpp
  auto serialize(serializer&, bool synchronize) -> void;
};

extern System system;

auto Region::NTSCJ() -> bool { return system.region() == System::Region::NTSCJ; }
auto Region::NTSCU() -> bool { return system.region() == System::Region::NTSCU; }
auto Region::PAL()   -> bool { return system.region() == System::Region::PAL;   }
auto Region::Dendy() -> bool { return system.region() == System::Region::Dendy; }
