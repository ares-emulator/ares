//Hudson Soft HuC6260: Video Color Encoder

struct VCE {
  struct Debugger {
    //debugger.cpp
    auto load(VCE&, Node::Object) -> void;

    struct Memory {
      Node::Debugger::Memory cram;
    } memory;
  } debugger;

  auto clock() const -> u32 { return io.clock; }
  auto width() const -> u32 { return io.clock == 4 ? 256 : io.clock == 3 ? 344 : 512; }

  //vce.cpp
  auto read(n3 address) -> n8;
  auto write(n3 address, n8 data) -> void;
  auto power() -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  struct CRAM {
    //vce.cpp
    auto read(n9 address) -> n9;
    auto write(n9 address, n1 a0, n8 data) -> void;

    n9 memory[0x200];
    n9 address;
  } cram;

  struct IO {
    n8 clock = 4;
    n1 extraLine;
    n1 grayscale;
  } io;
};
