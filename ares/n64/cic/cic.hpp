
struct CIC {
  enum State : u32 { BootRegion, BootSeed, BootChecksum, Run };
  enum Region : u32 { NTSC, PAL };

  nall::queue<n4> fifo;
  n8 seed;
  n48 checksum; //ipl2 checksum
  n1 type;
  n1 region;
  u32 state;

  //cic.cpp
  auto power(bool reset) -> void;
  auto read() -> n4;
  auto write(n2 cmd) -> void;
  auto poll() -> void;
  auto scramble(n4 *buf, int size) -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;
};

extern CIC cic;
