struct System {
  Node::System node;
  VFS::Pak pak;

  enum class Model : u32 { PCEngine, PCEngineDuo, SuperGrafx, LaserActive};
  enum class Region : u32 { NTSCJ, NTSCU };

  auto name() const -> string { return information.name; }
  auto model() const -> Model { return information.model; }
  auto region() const -> Region { return information.region; }
  auto colorburst() const -> f64 { return information.colorburst; }

  //system.cpp
  auto game() -> string;
  auto run() -> void;

  auto load(Node::System& node, string name) -> bool;
  auto save() -> void;
  auto unload() -> void;
  auto power(bool reset = false) -> void;

  //serialization.cpp
  auto serialize(bool synchronize) -> serializer;
  auto unserialize(serializer&) -> bool;

private:
  struct Information {
    string name = "PC Engine";
    Model model = Model::PCEngine;
    Region region = Region::NTSCU;  //more compatible
    f64 colorburst = Constants::Colorburst::NTSC;
  } information;

  //serialization.cpp
  auto serialize(serializer&, bool synchronize) -> void;
};

extern System system;

auto Model::PCEngine()    -> bool { return system.model() == System::Model::PCEngine;    }
auto Model::PCEngineDuo() -> bool { return (system.model() == System::Model::PCEngineDuo) || (system.model() == System::Model::LaserActive); }
auto Model::LaserActive() -> bool { return system.model() == System::Model::LaserActive; }
auto Model::SuperGrafx()  -> bool { return system.model() == System::Model::SuperGrafx;  }

auto Region::NTSCJ() -> bool { return system.region() == System::Region::NTSCJ; }
auto Region::NTSCU() -> bool { return system.region() == System::Region::NTSCU; }
