//an AY-3-8910 compatible block inside the system ASIC rather than a real AY.
//the two outer channels are panned hard left and right and the third sits in
//the middle at half volume, which is why the machine has a stereo headphone jack
struct PSG : AY38910, Thread {
  Node::Object node;
  Node::Audio::Stream stream;

  //psg.cpp
  auto load(Node::Object) -> void;
  auto unload() -> void;

  auto main() -> void;
  auto step(u32 clocks) -> void;
  auto power() -> void;

  auto read(n4 address) -> n8;
  auto write(n4 address, n8 data) -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

private:
  f64 volume[16];
};

extern PSG psg;
