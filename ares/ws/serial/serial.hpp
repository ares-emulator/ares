struct Serial : Thread, IO {
  Node::Object node;
  
  //serial.cpp
  auto load(Node::Object) -> void;
  auto unload() -> void;

  auto main() -> void;
  auto step(u32 clocks) -> void;

  auto readIO(n16 address) -> n8 override;
  auto writeIO(n16 address, n8 data) -> void override;

  auto power() -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  struct Debugger {
    Serial& self;

    //debugger.cpp
    auto load(Node::Object) -> void;
    auto unload(Node::Object) -> void;
    auto ports() -> string;

    struct Properties {
      Node::Debugger::Properties ports;
    } properties;
  } debugger{*this};

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
