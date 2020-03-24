struct Cartridge;

struct CartridgeSlot {
  CartridgeSlot(string_view name);
  auto load(Node::Object, Node::Object) -> void;
  auto unload() -> void;
  auto connect(Node::Peripheral) -> void;
  auto disconnect() -> void;

  const string name;
  Node::Port port;
  maybe<Cartridge&> peripheral;
};

extern CartridgeSlot cartridgeSlot;
