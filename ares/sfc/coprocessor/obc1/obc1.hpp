struct OBC1 {
  auto unload() -> void;
  auto power() -> void;

  auto read(uint24 address, uint8 data) -> uint8;
  auto write(uint24 address, uint8 data) -> void;

  auto serialize(serializer&) -> void;

  WritableMemory ram;

private:
  auto ramRead(uint address) -> uint8;
  auto ramWrite(uint address, uint8 data) -> void;

  struct {
    uint16 address;
    uint16 baseptr;
    uint16 shift;
  } status;
};

extern OBC1 obc1;
