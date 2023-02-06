
struct CIC {
  enum State : u32 { BootRegion, BootSeed, BootChecksum, Run, Challenge, Dead };
  enum Region : u32 { NTSC, PAL };
  enum ChallengeAlgo : bool { DummyChallenge, RealChallenge };

  nall::queue<n4> fifo;
  n8 seed;
  n48 checksum; //ipl2 checksum
  n1 type;
  n1 region;
  n1 challengeAlgo;
  u32 state;

  //cic.cpp
  auto power(bool reset) -> void;
  auto read() -> n4;
  auto write(n4 cmd) -> void;
  auto poll() -> void;
  auto scramble(n4 *buf, int size) -> void;
  
  //commands.cpp
  auto cmdCompare() -> void;
  auto cmdDie() -> void;
  auto cmdChallenge() -> void;
  auto cmdReset() -> void;
  auto challenge(n4 data[30]) -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;
};

extern CIC cic;
