struct POKEY {
  Node::Object node;

  auto load(Node::Object parent) -> void;
  auto unload() -> void;
  auto power() -> void;

  auto read(n8 address) -> n8;
  auto peek(n8 address) const -> n8;
  auto write(n8 address, n8 data) -> void;

};

extern POKEY pokey;
