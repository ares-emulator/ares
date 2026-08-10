struct GTIA {
  Node::Object node;

  auto load(Node::Object parent) -> void;
  auto unload() -> void;
  auto power() -> void;

  auto read(n8 address) -> n8;
  auto peek(n8 address) const -> n8;
  auto write(n8 address, n8 data) -> void;

  auto clock(n3 an) -> void;
  auto loadPlayerDMA(u8 player, n8 data, u32 scanline) -> void;
  auto loadMissileDMA(n8 data, u32 scanline) -> void;
  auto frame() -> void;

};

extern GTIA gtia;
