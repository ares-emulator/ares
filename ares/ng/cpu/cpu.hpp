//Motorola 68000

struct CPU : M68K, Thread {
  Node::Object node;

  //cpu.cpp
  auto load(Node::Object) -> void;
  auto unload() -> void;

  auto main() -> void;
  auto step(u32 clocks) -> void;
  auto power(bool reset) -> void;
  auto idle(u32 clocks) -> void override {}
  auto wait(u32 clocks) -> void override {}

  auto read(n1 upper, n1 lower, n24 address, n16 data = 0) -> n16 override { return 0; }
  auto write(n1 upper, n1 lower, n24 address, n16 data) -> void override {}

  //serialization.cpp
  auto serialize(serializer&) -> void;
};

extern CPU cpu;
