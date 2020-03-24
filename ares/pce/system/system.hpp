struct System {
  Node::Object node;

  enum class Model : uint { PCEngine, SuperGrafx };

  inline auto model() const -> Model { return information.model; }
  inline auto colorburst() const -> double { return information.colorburst; }

  //system.cpp
  auto run() -> void;

  auto load(Node::Object&, Node::Object) -> void;
  auto save() -> void;
  auto unload() -> void;
  auto power() -> void;

  //serialization.cpp
  auto serialize(bool synchronize) -> serializer;
  auto unserialize(serializer&) -> bool;

private:
  struct Information {
    Model model = Model::PCEngine;
    double colorburst = Constants::Colorburst::NTSC;
    uint32 serializeSize[2];
  } information;

  //serialization.cpp
  auto serialize(serializer&) -> void;
  auto serializeAll(serializer&, bool synchronize) -> void;
  auto serializeInit(bool synchronize) -> uint;
};

extern System system;

auto Model::PCEngine() -> bool { return system.model() == System::Model::PCEngine; }
auto Model::SuperGrafx() -> bool { return system.model() == System::Model::SuperGrafx; }
