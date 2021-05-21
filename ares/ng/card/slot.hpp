struct CardSlot {
  Node::Port port;
  unique_pointer<Card> device;

  //slot.cpp
  auto load(Node::Object) -> void;
  auto unload() -> void;
  auto allocate(string name) -> Node::Peripheral;

  CardSlot(string name);
  auto connect(Node::Peripheral) -> void;
  auto disconnect() -> void;

  auto read(n24 address) -> n8;
  auto write(n24 address, n8 data) -> void;

  auto save() -> void;
  auto power(bool reset) -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  n2 lock;
  n1 select;
  n3 bank;

protected:
  const string name;
  friend class Card;
};

extern CardSlot cardSlot;
