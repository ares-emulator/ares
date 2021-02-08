struct PSG : AY38910, Thread {
  Node::Object node;
  Node::Audio::Stream stream;

  //psg.cpp
  auto load(Node::Object) -> void;
  auto unload() -> void;

  auto main() -> void;
  auto step(u32 clocks) -> void;
  auto power() -> void;

  auto readIO(n1 port) -> n8 override;
  auto writeIO(n1 port, n8 data) -> void override;

  //serialization.cpp
  auto serialize(serializer&) -> void;

private:
  f64 volume[16];
};

extern PSG psg;
