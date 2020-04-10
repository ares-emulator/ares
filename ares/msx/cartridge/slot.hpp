struct CartridgeSlot {
  Node::Port port;

  //slot.cpp
  CartridgeSlot(string name);
  auto load(Node::Object) -> void;
  auto unload() -> void;
  auto connected() const -> bool;
  auto connect(Node::Peripheral) -> void;
  auto disconnect() -> void;

  const string name;
  Cartridge cartridge;
};

extern CartridgeSlot cartridgeSlot;
extern CartridgeSlot expansionSlot;
