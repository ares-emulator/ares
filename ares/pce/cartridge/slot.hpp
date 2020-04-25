struct CartridgeSlot {
  Node::Port port;
  Cartridge cartridge;

  //slot.cpp
  CartridgeSlot(string name);
  auto load(Node::Object) -> void;
  auto unload() -> void;

  auto power() -> void;
  auto serialize(serializer&) -> void;

  const string name;
};

extern CartridgeSlot cartridgeSlot;
