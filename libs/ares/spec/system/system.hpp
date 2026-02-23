struct ROM {
  Memory::Readable<u8> bios;
  Memory::Readable<u8> sub;
};

struct System {
  Node::System node;
  VFS::Pak pak;

  enum class Model : uint { Spectrum48k, Spectrum128 };

  auto model() const -> Model { return information.model; }
  auto frequency() const -> double { return information.frequency; }

  //system.cpp
  auto run() -> void;
  auto game() -> string;
  auto load(Node::System&, string name) -> bool;
  auto save() -> void;
  auto unload() -> void;
  auto power(bool reset) -> void;

  //serialization.cpp
  auto serialize(bool synchronize) -> serializer;
  auto unserialize(serializer&) -> bool;

  // 128k specific state
  bool romBank;
  bool screenBank;
  u3 ramBank;
  bool pagingDisabled;
private:
  struct Information {
    string name = "ZX Spectrum";
    Model model = Model::Spectrum48k;
    double frequency = 3'500'000;
    u32 serializeSize[2];
  } information;

  //serialization.cpp
  auto serialize(serializer&) -> void;
  auto serializeAll(serializer&, bool synchronize) -> void;
  auto serializeInit(bool synchronize) -> uint;
};

extern ROM rom;
extern System system;

auto Model::Spectrum48k() -> bool { return system.model() == System::Model::Spectrum48k; }
auto Model::Spectrum128() -> bool { return system.model() == System::Model::Spectrum128; }

