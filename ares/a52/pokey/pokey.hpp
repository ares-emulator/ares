struct POKEY : Thread {
  Node::Object node;

  //pokey.cpp
  auto load(Node::Object parent) -> void;
  auto unload() -> void;
  auto main() -> void;
  auto step(u32 clocks = 1) -> void;
  auto power() -> void;

  //io.cpp
  auto read(n8 address) -> n8;
  auto peek(n8 address) const -> n8;
  auto write(n8 address, n8 data) -> void;

  //dac.cpp
  auto output() const -> f64;

  //serialization.cpp
  auto serialize(serializer&) -> void;

//private:
  struct KeyboardLines {
    n1 kr1;
    n1 kr2;
  };

  struct SerialInput {
    n1 data;
    n1 clock;
  };

  //pokey.cpp
  auto sampleKeyboard(n6 scanAddress) -> KeyboardLines;
  auto clockKeyboard(bool clock15) -> void;

  //output.cpp
  auto powerOutput() -> void;
  auto clockOutput(f64 input) -> void;

  //io.cpp
  auto setIRQ() -> void;
  auto startPots() -> void;
  auto writeSKCTL(n8 data) -> void;

  Node::Audio::Stream stream;
  f64 outputDecay;
  f64 previousInput;
  f64 coupledOutput;

  // Altirra measured a 1.8 ms half-decay on Atari 8-bit hardware, equivalent
  // to this time constant. The Atari 5200 uses the same POKEY and a comparable
  // AC-coupled output path, so this is a provisional cross-machine parameter
  // until the post-coupling node is measured on an identified A5200 board.
  // Source: Altirra 4.40, src/ATAudio/source/pokey.cpp, UpdateMixTable().
  static constexpr f64 OutputTimeConstant = 0.002596851073600134;

  struct Clock {
    struct Pulses {
      bool clock64;
      bool clock15;
    };

    //clock.cpp
    auto power() -> void;
    auto clock(bool enabled) -> Pulses;
    auto random(bool usePolynomial9) const -> u8;
    auto sample4(u32 phase) const -> bool;
    auto sample5(u32 phase) const -> bool;
    auto sample9(u32 phase) const -> bool;
    auto sample17(u32 phase) const -> bool;

    //serialization.cpp
    auto serialize(serializer&) -> void;

  private:
    friend struct POKEY;

    //clock.cpp
    auto reset() -> void;
    auto advance() -> void;

    u32 prescaler64;
    u32 prescaler15;
    u32 polynomial4;
    u32 polynomial5;
    u32 polynomial9;
    u32 polynomial17;
    u32 polynomial4History;
    u32 polynomial5History;
    u32 polynomial9History;
    u32 polynomial17History;
  } clock;

  struct Audio {
    POKEY& self;
    Audio(POKEY& self) : self(self) {}

    struct Levels {
      n4 channel[4];
    };

    struct TimerEdges {
      n1 timer1;
      n1 timer2;
      n1 timer4;
    };

    //audio.cpp
    auto power() -> void;
    auto clockFilters() -> void;
    auto clockTimers(const Clock::Pulses&, const Clock&, bool holdTimer34) -> TimerEdges;
    auto writeFrequency(u32 channel, n8 data) -> void;
    auto writeControl(u32 channel, n8 data) -> void;
    auto writeAUDCTL(n8 data) -> void;
    auto startTimers() -> void;
    auto resetSerialTimer34() -> void;
    auto scheduleTwoToneResynchronization(u32 timerEvents) -> void;
    auto levels() const -> Levels;

    //serialization.cpp
    auto serialize(serializer&) -> void;

  private:
    friend struct POKEY;

    struct PipelineEvents {
      u32 channels;
      u32 reloads;
    };

    struct Channel {
      n8 frequency;
      n8 control;
      u32 counter;
      u32 eventDelay;
      u32 reloadDelay;
      n1 output;
      n1 filterLatch;
      n1 filterSample;
      u32 filterDelay;
    } channel[4];

    //audio.cpp
    auto channelPeriod(u32 index) const -> u32;
    auto channelIsJoined(u32 index) const -> bool;
    auto channelIsJoinedHigh(u32 index) const -> bool;
    auto channelUsesFastClock(u32 index) const -> bool;
    auto clockTimerCounter(u32 index, const Clock&) -> bool;
    auto clockChannel(u32 index, const Clock&) -> void;
    auto clockChannelSource(u32 index, const Clock&) -> void;
    auto sampleChannelFilters(u32 index) -> void;
    auto requestChannelIRQ(u32 index) -> void;
    auto clockJoinedCounters(u32 index) -> void;
    auto advanceTimerPipelines(const Clock&, bool holdTimer34) -> PipelineEvents;
    auto advanceTwoToneResynchronization() -> void;
    auto resynchronizeTimer(u32 index) -> void;
    auto resynchronizeTimer12() -> void;
    auto reloadTimers() -> void;

    n8 control;
    n1 timersRunning;
    u32 startDelay;
    n8 twoToneResyncPipeline;
  } audio{*this};

  struct IRQ {
    enum Source : u32 {
      Timer1,
      Timer2,
      Timer4,
      SerialComplete,
      SerialOutput,
      SerialInput,
      Keyboard,
      Break,
    };

    //irq.cpp
    auto power() -> void;
    auto clock() -> void;
    auto request(Source source) -> void;
    auto acknowledge(Source source) -> void;
    auto pending(Source source) const -> bool;
    auto writeEnable(n8 data) -> void;
    auto status() const -> n8;
    auto line() const -> bool;

    //serialization.cpp
    auto serialize(serializer&) -> void;

  private:
    friend struct POKEY;

    n8 enable;
    n8 statusValue;
    u32 enabledAge[3];
    u32 disabledAge[3];
  } irq;

  struct Pots {
    //pots.cpp
    auto power() -> void;
    auto start(const n8 targets[8]) -> void;
    auto clockFast() -> void;
    auto clock15() -> void;
    auto read(u32 index) const -> n8;
    auto all() const -> n8;

    //serialization.cpp
    auto serialize(serializer&) -> void;

  private:
    friend struct POKEY;

    static constexpr u32 Maximum = 228;
    static constexpr u32 FastMaximum = 229;
    static constexpr u32 FastStop = 231;

    //pots.cpp
    auto advance() -> void;

    n8 value[8];
    n8 target[8];
    n8 allValue;
    u32 counter;
    n1 scanning;
  } pots;

  struct Status {
    //status.cpp
    auto power() -> void;
    auto resetErrors() -> void;
    auto setKeyboardDown(bool active) -> void;
    auto setShift(bool active) -> void;
    auto setKeyboardOverrun() -> void;
    auto setSerialInput(bool high) -> void;
    auto setSerialBusy(bool active) -> void;
    auto setSerialOutput(bool high) -> void;
    auto setSerialInputOverrun() -> void;
    auto setSerialFrameError() -> void;
    auto read() const -> n8;

    //serialization.cpp
    auto serialize(serializer&) -> void;

  private:
    friend struct POKEY;

    n8 value;
  } status;

  struct Keyboard {
    POKEY& self;
    Keyboard(POKEY& self) : self(self) {}

    //keyboard.cpp
    auto power() -> void;
    auto disable() -> void;
    auto clock(bool debounce) -> void;
    auto code() const -> n8;

    //serialization.cpp
    auto serialize(serializer&) -> void;

  private:
    friend struct POKEY;

    enum class State : u8 {
      Released,
      DebouncePress,
      Pressed,
      DebounceRelease,
    };

    n8 codeValue;
    n6 scanCounter;
    n6 compareValue;
    State state;
    n1 shiftLatch;
    n1 controlLatch;
    n1 breakLatch;
  } keyboard{*this};

  struct Serial {
    POKEY& self;
    Serial(POKEY& self) : self(self) {}

    //serial.cpp
    auto power() -> void;
    auto write(n8 data) -> void;
    auto configure() -> void;
    auto clock(const Audio::TimerEdges&, SerialInput) -> void;
    auto reset() -> void;
    auto input() const -> n8;
    auto holdsTimer34() const -> bool;

    //serialization.cpp
    auto serialize(serializer&) -> void;

  //private:
    friend struct POKEY;

    struct LineEdges {
      bool inputFalling;
      bool clockFalling;
      bool clockRising;
    };

    //serial.cpp
    auto observeLines(SerialInput) -> LineEdges;
    auto clockInputPath(const Audio::TimerEdges&, bool data, const LineEdges&) -> void;
    auto clockOutputPath(const Audio::TimerEdges&, const LineEdges&) -> void;
    auto clockTwoTone(const Audio::TimerEdges&) -> void;
    auto clockInput(bool data) -> void;
    auto clockOutput() -> void;
    auto loadOutput() -> void;
    auto currentOutput() const -> bool;
    auto mode() const -> n3;

    n8 inputValue;
    n10 inputShifter;
    n4 inputProgress;
    n8 outputHoldingValue;
    n1 outputHoldingFull;
    n10 outputShifter;
    n4 outputProgress;
    n1 inputClockPhase;
    n1 outputClockPhase;
    n1 outputLine;
    n1 twoToneOutput;
    n1 asynchronousReceiving;
    n1 previousInputLine;
    n1 previousExternalClock;
  } serial{*this};

  SerialInput serialInput{1, 1};
  n8 control;
};

extern POKEY pokey;
