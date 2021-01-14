struct System {
  Node::System node;
  Node::Setting::String regionNode;

  enum class Model : uint { PCEngine, PCEngineDuo, SuperGrafx };
  enum class Region : uint { NTSCJ, NTSCU };

  auto name() const -> string { return node->name(); }
  auto model() const -> Model { return information.model; }
  auto region() const -> Region { return information.region; }
  auto colorburst() const -> double { return information.colorburst; }

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
    Model model = Model::PCEngine;
    Region region = Region::NTSCU;  //more compatible
    double colorburst = Constants::Colorburst::NTSC;
  } information;

  //serialization.cpp
  auto serialize(serializer&, bool synchronize) -> void;
};

extern System system;

auto Model::PCEngine()    -> bool { return system.model() == System::Model::PCEngine;    }
auto Model::PCEngineDuo() -> bool { return system.model() == System::Model::PCEngineDuo; }
auto Model::SuperGrafx()  -> bool { return system.model() == System::Model::SuperGrafx;  }

auto Region::NTSCJ() -> bool { return system.region() == System::Region::NTSCJ; }
auto Region::NTSCU() -> bool { return system.region() == System::Region::NTSCU; }
