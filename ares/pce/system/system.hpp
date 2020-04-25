struct System {
  Node::Object node;
  Node::String regionNode;

  enum class Model : uint { PCEngine, PCEngineDuo, SuperGrafx };
  enum class Region : uint { NTSCJ, NTSCU };

  auto model() const -> Model { return information.model; }
  auto region() const -> Region { return information.region; }
  auto colorburst() const -> double { return information.colorburst; }

  //system.cpp
  auto run() -> void;

  auto load(Node::Object&) -> void;
  auto save() -> void;
  auto unload() -> void;
  auto power() -> void;

  //serialization.cpp
  auto serialize(bool synchronize) -> serializer;
  auto unserialize(serializer&) -> bool;

private:
  struct Information {
    Model model = Model::PCEngine;
    Region region = Region::NTSCU;  //more compatible
    double colorburst = Constants::Colorburst::NTSC;
    uint32 serializeSize[2];
  } information;

  //serialization.cpp
  auto serialize(serializer&) -> void;
  auto serializeAll(serializer&, bool synchronize) -> void;
  auto serializeInit(bool synchronize) -> uint;
};

extern System system;

auto Model::PCEngine()    -> bool { return system.model() == System::Model::PCEngine;    }
auto Model::PCEngineDuo() -> bool { return system.model() == System::Model::PCEngineDuo; }
auto Model::SuperGrafx()  -> bool { return system.model() == System::Model::SuperGrafx;  }

auto Region::NTSCJ() -> bool { return system.region() == System::Region::NTSCJ; }
auto Region::NTSCU() -> bool { return system.region() == System::Region::NTSCU; }
