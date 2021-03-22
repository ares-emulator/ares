//Zilog Z80

struct APU : Z80, Z80::Bus, Thread {
  Node::Object node;

  auto synchronizing() const -> bool override { return false; }

  //z80.cpp
  auto load(Node::Object) -> void;
  auto unload() -> void;

  auto main() -> void;
  auto step(u32 clocks) -> void override;
  auto power(bool reset) -> void;

  auto read(n16 address) -> n8 override { return 0; }
  auto write(n16 address, n8 data) -> void override {}

  auto in(n16 address) -> n8 override { return 0; }
  auto out(n16 address, n8 data) -> void override { return; }

  //serialization.cpp
  auto serialize(serializer&) -> void;
};

extern APU apu;
