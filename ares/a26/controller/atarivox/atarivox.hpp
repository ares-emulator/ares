#include "speakjet.hpp"

struct AtariVox : SaveKey, Thread {
  AtariVox(Node::Port);
  ~AtariVox();

  auto main() -> void;
  auto power(bool reset) -> void override;
  auto read() -> n8 override;
  auto write(n8 data) -> void override;
  auto serialize(serializer&) -> void override;

private:
  auto receiveSerial(bool level) -> void;

  Node::Audio::Stream stream;
  SpeakJet speakjet;

  n4 serialBits;
  n8 serialData;
  n1 serialValid;
  u64 serialCycle;
};
