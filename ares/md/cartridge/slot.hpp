struct CartridgeSlot {
  Node::Port port;
  Cartridge cartridge;

  auto connected() const -> bool { return (bool)cartridge.node; }

  //slot.cpp
  CartridgeSlot(string name);
  auto load(Node::Object) -> void;
  auto unload() -> void;

  const string name;
};

extern CartridgeSlot cartridgeSlot;
