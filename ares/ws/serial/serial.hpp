struct Serial : Thread, IO {
  //serial.cpp
  auto main() -> void;
  auto step(u32 clocks) -> void;

  auto readIO(n16 address) -> n8 override;
  auto writeIO(n16 address, n8 data) -> void override;

  auto power() -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  struct State {
    n8 dataTx;
    n8 dataRx;
    n1 baudRate;  //0 = 9600; 1 = 38400
    n1 enable;
    n1 rxOverrun;
    n1 txFull;
    n1 rxFull;
    n8 baudClock;
    n8 txBitClock;
    n8 rxBitClock;
  } state;
};

extern Serial serial;