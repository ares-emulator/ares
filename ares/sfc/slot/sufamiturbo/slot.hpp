struct SufamiTurboSlot {
  Node::Port port;

  //slot.cpp
  SufamiTurboSlot(string name);
  auto load(Node::Object) -> void;
  auto unload() -> void;
  auto connected() const -> bool;
  auto connect(Node::Peripheral) -> void;
  auto disconnect() -> void;

  const string name;
  SufamiTurboCartridge cartridge;
};

extern SufamiTurboSlot sufamiturboSlotA;
extern SufamiTurboSlot sufamiturboSlotB;
