//Mega CD interface:
//the only actual peripheral released for the expansion port was the Mega CD.
//this class is used to connect a peripheral folder and the BIOS to the emulator.

struct Expansion {
  Node::Port port;
  Node::Peripheral node;

  inline auto manifest() const -> string { return information.manifest; }
  inline auto name() const -> string { return information.name; }
  inline auto regions() const -> vector<string> { return information.regions; }

  auto load(Node::Object, Node::Object) -> void;
  auto unload() -> void;
  auto connect(Node::Peripheral) -> void;
  auto disconnect() -> void;

  auto save() -> void;
  auto power() -> void;

  auto read(uint1 upper, uint1 lower, uint22 address, uint16 data) -> uint16;
  auto write(uint1 upper, uint1 lower, uint22 address, uint16 data) -> void;

  auto readIO(uint1 upper, uint1 lower, uint24 address, uint16 data) -> uint16;
  auto writeIO(uint1 upper, uint1 lower, uint24 address, uint16 data) -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  struct Information {
    string manifest;
    string name;
    vector<string> regions;
  };

  Information information;
};

extern Expansion expansion;
