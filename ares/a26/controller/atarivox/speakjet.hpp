struct SpeakJet {
  static constexpr f64 Frequency = 8192.0;

  enum class SoundClass : u8 {
    None,
    Vowel,
    Nasal,
    Resonate,
    Affricate,
    Fricative,
    Stop,
    Robot,
    Alarm,
    Beep,
    Biological,
    DTMF,
    Sonar,
    Pistol,
    Wow,
    Pause,
  };

  auto reset() -> void;
  auto enqueue(u8 data) -> bool;
  auto ready() const -> bool { return fifoCount < 64; }
  auto buffered() const -> u32 { return fifoCount; }
  auto speaking() const -> bool { return soundRemaining; }
  auto clock() -> f64;
  auto serialize(serializer&) -> void;

  static auto duration(u8 code) -> u32;
  static auto classify(u8 code) -> SoundClass;

private:
  auto dequeue() -> u8;
  auto processCommands() -> void;
  auto execute(u8 code, bool repeated = false) -> void;
  auto parameter(u8 command, u8 value) -> void;
  auto startSound(u8 code) -> void;
  auto finishSound() -> void;
  auto synthesize() -> f64;
  auto synthesizeSpeech(f64 progress) -> f64;
  auto synthesizeEffect(f64 progress) -> f64;
  auto resonator(u32 index, f64 input, f64 frequency, f64 bandwidth) -> f64;
  auto oscillator(u32 index, f64 frequency) -> f64;
  auto noise() -> f64;

  u8 fifo[64] = {};
  n6 fifoRead;
  n6 fifoWrite;
  n7 fifoCount;

  n1 parameterPending;
  u8 pendingCommand;
  n1 waiting;
  u8 volume;
  u8 speed;
  u8 pitch;
  u8 bend;
  n3 portControl;
  n3 portOutput;
  u8 repeatRequest;
  u8 repeatCode;
  u8 repeatRemaining;
  f64 nextDurationScale;
  f64 nextPitchScale;

  u8 soundCode;
  SoundClass soundClass;
  u32 soundSamples;
  u32 soundRemaining;
  f64 soundPitchScale;
  f64 phase[5] = {};
  f64 filter[3][2] = {};
  u16 noiseLfsr;
};
