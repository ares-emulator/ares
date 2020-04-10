struct PSG : AY38910, Thread {
  Node::Component node;
  Node::Stream stream;

  //psg.cpp
  auto load(Node::Object) -> void;
  auto unload() -> void;

  auto main() -> void;
  auto step(uint clocks) -> void;
  auto power() -> void;

  auto readIO(uint1 port) -> uint8 override;
  auto writeIO(uint1 port, uint8 data) -> void override;

  //serialization.cpp
  auto serialize(serializer&) -> void;

private:
  double volume[16];
};

extern PSG psg;
