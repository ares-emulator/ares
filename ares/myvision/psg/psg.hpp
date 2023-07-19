struct PSG : AY38910, Thread {
  Node::Object node;
  Node::Audio::Stream stream;

  //psg.cpp
  auto load(Node::Object) -> void;
  auto unload() -> void;

  auto main() -> void;
  auto step(uint clocks) -> void;
  auto power() -> void;

  auto readIO(n1 port) -> n8 override;
  auto writeIO(n1 port, n8 data) -> void override;

  //serialization.cpp
  auto serialize(serializer&) -> void;

private:
  double volume[16];
  n8 column = 0;
};

extern PSG psg;
