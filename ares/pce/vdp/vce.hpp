//Hudson Soft HuC6260: Video Color Encoder

struct VCE {
  inline auto clock() const -> uint { return io.clock; }

  //vce.cpp
  auto read(uint3 address) -> uint8;
  auto write(uint3 address, uint8 data) -> void;
  auto power() -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  struct CRAM {
    //vce.cpp
    auto read(uint9 address) -> uint9;
    auto write(uint9 address, uint1 a0, uint8 data) -> void;

    uint9 memory[0x200];
    uint9 address;
  } cram;

  struct IO {
    uint8 clock = 4;
    uint1 extraLine;
    uint1 grayscale;
  } io;
};
