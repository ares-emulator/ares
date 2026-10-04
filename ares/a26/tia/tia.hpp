struct TIA : Thread {
  Node::Object node;

  struct ObjectSignals {
    n1 player[2];
    n1 missile[2];
    n1 ball;
  };

  auto horizontalCounter() const -> u8 { return timing.hcounter; }
  auto displayPosition() const -> i16 { return timing.position(); }

  //tia.cpp
  auto load(Node::Object) -> void;
  auto unload() -> void;

  auto main() -> void;
  auto step(u32 clocks = 1) -> void;
  auto power(bool reset) -> void;
  auto scanline() -> void;
  auto clock() -> bool;
  auto objectResetCounter() const -> u8;

  //write-queue.cpp
  auto queueWrite(u8 address, n8 data, i8 delay) -> void;
  auto clockWrites() -> void;
  auto commitWrite(u8 address, n8 data) -> void;

  //analog.cpp
  auto updateAnalogInput(n2 index) -> void;

  //trigger.cpp
  auto updateTriggerInput(n1 index) -> void;
  auto readTrigger(n1 index) -> n1;

  //io.cpp
  auto read(n8 address, n8 data) -> n8;
  auto write(n8 address, n8 data) -> void;
  auto writeVblank(n8 data) -> void;
  auto setVsync(n1 state) -> void;
  auto rsync() -> void;
  auto ctrlpf(n8 data) -> void;
  auto hmove() -> void;
  auto grp(n1 index, n8 data) -> void;
  auto resp(n1 index) -> void;
  auto resm(n1 index) -> void;
  auto resmp(n1 index, n8 data) -> void;
  auto resbl() -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  struct Timing {
    auto position() const -> i16 { return (i16)hcounter - hcounterDelta; }
    auto horizontalBlank() const -> n1 {
      return (extendedHblank && position() < 76) || position() < 68;
    }

    //timing.cpp
    auto rsync() -> void;
    auto advance() -> bool;

    //serialization.cpp
    auto serialize(serializer&) -> void;

    u8 hcounter;
    i16 hcounterDelta;
    n1 extendedHblank;
  } timing;

  struct DelayedWrite {
    //serialization.cpp
    auto serialize(serializer&) -> void;

    n1 active;
    u8 address;
    n8 data;
    i8 delay;
  } writes[16];

  n1 vsync;
  n1 vblank;

  struct ObjectPipeline {
    using Signals = ObjectSignals;

    struct Missile {
      //missile.cpp
      auto clock(u8 cycles = 1, n1 regularClock = 1, u8 hcounter = 0) -> void;
      auto latchOutput() -> void;
      auto start(n2 copy) -> void;
      auto reset(u8 counter, n1 hblank) -> void;
      auto nusiz(n8 data) -> void;
      auto width() -> u8;

      //serialization.cpp
      auto serialize(serializer&) -> void;

      n1 enable;
      n1 lockedToPlayer;
      n3 copies;
      n2 size;
      n4 offset;
      n1 moving;
      i9 counter;
      i8 renderCounter;
      u8 effectiveWidth;
      n1 rendering;
      n1 output;
      n2 copy;
    };

    struct Player {
      //player.cpp
      auto clock(u8 cycles = 1) -> n1;
      auto latchOutput() -> void;
      auto start(n2 copy) -> void;
      auto reset(u8 counter) -> void;
      auto nusiz(n3 size, n1 hblank) -> void;
      auto setDivider(u8 divider) -> void;
      auto missileResetCounter() const -> u8;
      auto width() -> u8;

      //serialization.cpp
      auto serialize(serializer&) -> void;

      n8 graphics[2];
      n1 reflect;
      n3 size;
      n4 offset;
      n1 moving;
      n1 delay;
      i9 counter;
      i8 renderCounter;
      i8 renderCounterTripPoint;
      u8 sampleCounter;
      i8 dividerChangeCounter;
      u8 divider;
      u8 dividerPending;
      n1 rendering;
      n1 output;
      n2 copy;
    };

    struct Ball {
      //ball.cpp
      auto clock(u8 cycles = 1, n1 regularClock = 1) -> void;
      auto latchOutput() -> void;
      auto reset(u8 counter) -> void;

      //serialization.cpp
      auto serialize(serializer&) -> void;

      n1 enable[2];
      n2 size;
      n4 offset;
      n1 moving;
      n1 delay;
      i9 counter;
      i8 renderCounter;
      u8 effectiveWidth;
      i9 lastMovementCounter;
      n1 rendering;
      n1 output;
    };

    struct Pair {
      Player player;
      Missile missile;
    } pair[2];

    //objects.cpp
    auto player(n1 index) -> Player& { return pair[index].player; }
    auto missile(n1 index) -> Missile& { return pair[index].missile; }
    auto player(n1 index) const -> const Player& { return pair[index].player; }
    auto missile(n1 index) const -> const Missile& { return pair[index].missile; }
    auto clock(n1 hblank, u8 hcounter) -> Signals;
    auto movementClock(u32 phase, n1 hblank, u8 hcounter) -> void;
    auto runMovement(n1 hblank, u8 hcounter) -> void;
    auto movementActive() const -> n1;
    auto hmove() -> void;
    auto hmclr() -> void;
    static auto decode(n3 mode, u8 counter) -> u8;
    auto signals() const -> Signals;
    auto nusiz(n1 index, n8 data, n1 hblank) -> void;
    auto power() -> void;

    //serialization.cpp
    auto serialize(serializer&) -> void;

    n5 movementPhase;
    Ball ball;
  } objects;

  struct Playfield {
    //playfield.cpp
    auto clock(i16 x) -> n1;
    auto nextLine() -> void;

    //serialization.cpp
    auto serialize(serializer&) -> void;

    n20 graphics;
    n1 pixel;
    n1 mirror;
    n1 mirrorActive;
  } playfield;

  struct Priority {
    enum class Source : u32 { Background, Playfield, Ball, Player0, Player1 };

    //priority.cpp
    auto resolveSource(n1 playfield, const ObjectSignals&) const -> Source;
    auto resolveColor(Source, i16 x) const -> n7;

    //serialization.cpp
    auto serialize(serializer&) -> void;

    n7 backgroundColor;
    n7 playerColor[2];
    n7 playfieldColor;
    n1 scoreMode;
    n1 playfieldPriority;
  } priority;

  struct Collision {
    //collision.cpp
    auto clock(n1 playfield, const ObjectSignals&, n1 vblank) -> void;
    auto read(n8 address, n8 data) const -> n8;
    auto clear() -> void;
    auto power() -> void { clear(); }

    //serialization.cpp
    auto serialize(serializer&) -> void;

    n1 M0P0;
    n1 M0P1;
    n1 M1P0;
    n1 M1P1;
    n1 P0PF;
    n1 P0BL;
    n1 P1PF;
    n1 P1BL;
    n1 M0PF;
    n1 M0BL;
    n1 M1PF;
    n1 M1BL;
    n1 BLPF;
    n1 P0P1;
    n1 M0M1;
  } collision;

  struct Audio {
    Node::Audio::Stream stream;

    //audio.cpp
    auto load(Node::Object parent, f64 frequency) -> void;
    auto unload(Node::Object parent) -> void;
    auto clock() -> void;
    auto advance() -> void { if(++phase == 228) phase = 0; }
    auto dacConductance(u8 code) const -> f64;
    auto loadedOutput(f64 conductance) const -> f64;
    auto power() -> void;

    //serialization.cpp
    auto serialize(serializer&) -> void;

    struct Channel {
      //audio.cpp
      auto phase0() -> void;
      auto phase1() -> void;
      auto output() const -> u8;

      //serialization.cpp
      auto serialize(serializer&) -> void;

      n1 enable;
      n8 divCounter;
      n8 noiseCounter;
      n1 noiseFeedback;
      n8 pulseCounter;
      n1 pulseCounterPaused;
      n1 pulseFeedback;
      n4 volume;
      n4 control;
      n5 frequency;
    } channel[2];

    u8 phase;
    f64 sum;
    u8 clocks;
  } audio;

  struct TriggerInputs {
    struct Input {
      //serialization.cpp
      auto serialize(serializer&) -> void;

      n1 mode;
      n1 value;
    } input[2];

    //trigger.cpp
    auto sample(n1 index, n1 value) -> void;
    auto read(n1 index, n1 value) -> n1;
    auto vblank(n1 latch) -> void;
    auto power() -> void;

    //serialization.cpp
    auto serialize(serializer&) -> void;
  } triggers;

  struct AnalogInputs {
    struct Input {
      //serialization.cpp
      auto serialize(serializer&) -> void;

      f64 voltage;
      u64 timestamp;
      Controller::AnalogConnection connection;
    } input[4];

    //analog.cpp
    auto power(f64 frequency) -> void;
    auto advance() -> void;
    auto update(n2 index, Controller::AnalogConnection connection) -> void;
    auto vblank(n1 dumped) -> void;
    auto read(n2 index) -> n1;
    auto advance(Input& input) -> void;

    //serialization.cpp
    auto serialize(serializer&) -> void;

    static constexpr f64 SeriesResistance = 1'800.0;
    static constexpr f64 Capacitance = 68e-9;
    static constexpr f64 DumpResistance = 50.0;
    static constexpr f64 SupplyVoltage = 5.0;
    static constexpr f64 CalibrationResistance = 1'000'000.0;
    //Stella calibrates 1 Mohm to 379 scanlines.
    static constexpr f64 TripClocks = 379.0 * 228.0;

    u64 time;
    n1 dumped;
    f64 frequency;
    f64 tripVoltage;
  } analog;
};

extern TIA tia;
