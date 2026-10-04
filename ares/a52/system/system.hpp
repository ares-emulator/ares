extern Random random;

struct System {
  Node::System node;
  VFS::Pak pak;

  auto name() const -> string { return information.name; }
  auto frequency() const -> f64 { return information.frequency; }

  auto game() -> string;
  auto run() -> void;

  auto load(Node::System& node, string name) -> bool;
  auto save() -> void;
  auto unload() -> void;
  auto power(bool reset) -> void;

  //serialization.cpp
  auto serialize(bool synchronize) -> serializer;
  auto unserialize(serializer&) -> bool;
  auto serialize(serializer&, bool synchronize) -> void;

  Memory::Writable<n8> ram;
  Memory::Readable<n8> bios;

private:
  struct Information {
    string name = "Atari 5200";
    f64 frequency = Constants::Colorburst::NTSC;
  } information;
};

extern System system;
