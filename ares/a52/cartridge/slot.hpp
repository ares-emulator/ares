struct CartridgeSlot {
  Node::Port port;
  Cartridge cartridge;

  auto load(Node::Object parent) -> void;
  auto unload() -> void;
};

extern CartridgeSlot cartridgeSlot;
