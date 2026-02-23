struct OBC1 {
  auto unload() -> void;
  auto power() -> void;

  auto read(n24 address, n8 data) -> n8;
  auto write(n24 address, n8 data) -> void;

  auto serialize(serializer&) -> void;

  WritableMemory ram;

private:
  auto ramRead(n13 address) -> n8;
  auto ramWrite(n13 address, n8 data) -> void;

  struct {
    n16 address;
    n16 baseptr;
    n16 shift;
  } status;
};

extern OBC1 obc1;
