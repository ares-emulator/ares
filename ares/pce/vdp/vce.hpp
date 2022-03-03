//Hudson Soft HuC6260: Video Color Encoder

struct VCEBase {
  virtual auto read(n3 address) -> n8 = 0;
  virtual auto write(n3 address, n8 data) -> void = 0;
};

struct VCE : VCEBase {
  struct Debugger {
    //debugger.cpp
    auto load(VCE&, Node::Object) -> void;

    struct Memory {
      Node::Debugger::Memory cram;
    } memory;
  } debugger;

  auto clock() const -> u32 { return io.clock; }

  //vce.cpp
  auto read(n3 address) -> n8 override;
  auto write(n3 address, n8 data) -> void override;
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
