struct Bridge {
  struct Buffer {
    bool ready;
    uint8 data;
  };
  Buffer cputoarm;
  Buffer armtocpu;
  uint32 timer;
  uint32 timerlatch;
  bool reset;
  bool ready;
  bool signal;

  auto status() const -> uint8 {
    uint8 data;
    data.bit(0) = armtocpu.ready;
    data.bit(2) = signal;
    data.bit(3) = cputoarm.ready;
    data.bit(7) = ready;
    return data;
  }
} bridge;
