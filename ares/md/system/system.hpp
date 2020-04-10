extern Random random;

struct System {
  Node::Object node;
  Node::Boolean tmss;
  Node::String regionNode;

  struct Controls {
    Node::Object node;
    Node::Button reset;

    //controls.cpp
    auto load(Node::Object) -> void;
    auto poll() -> void;
  } controls;

  enum class Region : uint { NTSCJ, NTSCU, PAL };

  auto region() const -> Region { return information.region; }
  auto megaCD() const -> bool { return information.megaCD; }
  auto frequency() const -> double { return information.frequency; }

  auto run() -> void;

  auto load(Node::Object&) -> void;
  auto unload() -> void;
  auto save() -> void;
  auto power(bool reset) -> void;

  //serialization.cpp
  auto serialize(bool synchronize) -> serializer;
  auto unserialize(serializer&) -> bool;

private:
  struct Information {
    Region region = Region::NTSCJ;
    bool megaCD = false;
    double frequency = Constants::Colorburst::NTSC * 15.0;
    uint32 serializeSize[2];
  } information;

  //serialization.cpp
  auto serialize(serializer&) -> void;
  auto serializeAll(serializer&, bool synchronize) -> void;
  auto serializeInit(bool synchronize) -> uint;
};

extern System system;

auto Region::NTSCJ() -> bool { return system.region() == System::Region::NTSCJ; }
auto Region::NTSCU() -> bool { return system.region() == System::Region::NTSCU; }
auto Region::PAL() -> bool { return system.region() == System::Region::PAL; }

auto MegaCD() -> bool { return system.megaCD(); }
