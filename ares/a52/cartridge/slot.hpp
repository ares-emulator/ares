struct CartridgeSlot {
  Node::Port port;
  Cartridge cartridge;

  //slot.cpp
  auto load(Node::Object parent) -> void;
  auto unload() -> void;
};

extern CartridgeSlot cartridgeSlot;
