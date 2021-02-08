//Programmable Sound Generator

struct PSG : Thread {
  Node::Object node;
  Node::Audio::Stream stream;

  //psg.cpp
  auto load(Node::Object) -> void;
  auto unload() -> void;

  auto main() -> void;
  auto frame(i16&, i16&) -> void;
  auto step(u32 clocks) -> void;

  auto power() -> void;

  //io.cpp
  auto write(n4 address, n8 data) -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

private:
  struct IO {
    n3 channel;
    n4 volumeLeft;
    n4 volumeRight;
    n8 lfoFrequency;
    n2 lfoControl;
    n1 lfoEnable;
  } io;

  struct Channel {
    //channel.cpp
    auto power(u32 id) -> void;
    auto run() -> void;
    auto sample(n5 sample) -> void;

    //io.cpp
    auto write(n4 address, n8 data) -> void;

    struct IO {
      n12 waveFrequency;
      n5  volume;
      n1  direct;
      n1  enable;
      n4  volumeLeft;
      n4  volumeRight;
      n5  waveBuffer[32];
      n5  noiseFrequency;  //channels 4 and 5 only
      n1  noiseEnable;     //channels 4 and 5 only

      n12 wavePeriod;
      n5  waveSample;
      n5  waveOffset;
      n12 noisePeriod;
      n5  noiseSample;

      n5  output;
    } io;

    u32 id;
  } channel[6];

//unserialized:
  f64 volumeScalar[32];
};

extern PSG psg;
