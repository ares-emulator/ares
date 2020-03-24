struct Player : Thread {
  auto main() -> void;
  auto step(uint clocks) -> void;

  auto power() -> void;
  auto frame() -> void;

  auto keyinput() -> maybe<uint16>;
  auto read() -> maybe<uint32>;
  auto write(uint2 addr, uint8 byte) -> void;

  auto serialize(serializer& s) -> void;

private:
  struct Status {
     uint1 enable;
     uint1 rumble;

     uint1 logoDetected;
    uint32 logoCounter;

    uint32 packet;
    uint32 send;
    uint32 recv;

    uint32 timeout;
  } status;
};

extern Player player;
