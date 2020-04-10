struct CartridgeSlot {
  CartridgeSlot(string name);
  auto load(Node::Object) -> void;
  auto unload() -> void;
  auto connect(Node::Peripheral) -> void;
  auto disconnect() -> void;
  auto connected() const -> bool { return (bool)cartridge.node; }

  const string name;
  Node::Port port;
  Cartridge cartridge;
};

extern CartridgeSlot cartridgeSlot;
