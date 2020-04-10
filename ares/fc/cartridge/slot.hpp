struct CartridgeSlot {
  CartridgeSlot(string name);
  auto load(Node::Object) -> void;
  auto unload() -> void;
  auto connected() const -> bool;
  auto connect(Node::Peripheral) -> void;
  auto disconnect() -> void;

  const string name;
  Node::Port port;
  Cartridge cartridge;
};

extern CartridgeSlot cartridgeSlot;
