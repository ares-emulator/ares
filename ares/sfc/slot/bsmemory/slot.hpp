struct BSMemorySlot {
  Node::Port port;

  //slot.cpp
  BSMemorySlot(string name);
  auto load(Node::Object) -> void;
  auto unload() -> void;
  auto connected() const -> bool;
  auto connect(Node::Peripheral) -> void;
  auto disconnect() -> void;

  const string& name;
  BSMemoryCartridge cartridge;
};

extern BSMemorySlot bsmemorySlot;
