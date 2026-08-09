// GTIA: graphics television interface adapter
struct GTIA {
  Node::Object node;
  Node::Video::Screen screen;

  //gtia.cpp
  auto load(Node::Object parent) -> void;
  auto unload() -> void;
  auto power() -> void;

  //timing.cpp
  auto clock(n3 an) -> void;

  //io.cpp
  auto read(n8 address) -> n8;
  auto peek(n8 address) const -> n8;
  auto write(n8 address, n8 data) -> void;

  //player-missile.cpp
  auto loadPlayerDMA(u8 player, n8 data, u32 scanline) -> void;
  auto loadMissileDMA(n8 data, u32 scanline) -> void;

  //console.cpp
  auto controllerSelect() const -> u32;
  auto controllerPower() const -> bool;

  //video.cpp
  auto frame() -> void;

  //color.cpp
  auto color(n32 color) -> n64;

  static constexpr u32 OverscanViewportX = Timing::VisibleFirstColorClock * Timing::SamplesPerColorClock;
  static constexpr u32 OverscanViewportY = Timing::DisplayListFirstScanline;
  static constexpr u32 OverscanViewportWidth =
    (Timing::VisibleLastColorClock - Timing::VisibleFirstColorClock + 1) * Timing::SamplesPerColorClock;
  static constexpr u32 OverscanViewportHeight =
    Timing::DisplayListLastScanline - Timing::DisplayListFirstScanline + 1;
  static constexpr u32 NominalViewportWidth = 42 * 8;
  static constexpr u32 NominalViewportHeight = OverscanViewportHeight;
  static constexpr u32 NominalViewportX =
    OverscanViewportX + (OverscanViewportWidth - NominalViewportWidth) / 2;
  static constexpr u32 NominalViewportY = OverscanViewportY;

private:
  struct Sample {
    n3 an;
    n1 hblank;
    n1 vsync;
  };

  struct PlayerMissile {
    struct Signals {
      u8 players;
      u8 missiles;
    };

    //player-missile.cpp
    auto loadPlayerDMA(u8 player, n8 data, bool enabled, bool oddLine) -> void;
    auto loadMissileDMA(n8 data, bool enabled, bool oddLine) -> void;
    auto clock(u8 horizontal) -> Signals;
    auto clockRegisters() -> void;
    auto writePosition(u8 index, n8 data) -> void;
    auto writeGraphics(u8 index, n8 data) -> void;
    auto writePlayerSize(u8 index, n2 data) -> void;
    auto writeMissileSize(n8 data) -> void;
    auto writeVerticalDelay(n8 data) -> void;

  private:
    //player-missile.cpp
    auto clockPlayer(u8 index, u8 horizontal) -> bool;
    auto clockMissile(u8 index, u8 horizontal) -> bool;

  public:
    n8 playerPosition[4];
    n8 missilePosition[4];
    n2 playerSize[4];
    n8 missileSize;
    n8 playerGraphics[4];
    n8 missileGraphics;
    n8 verticalDelay;

    n8 playerShift[4];
    n2 missileShift[4];
    n4 playerRemaining[4];
    n2 missileRemaining[4];
    n2 playerStretch[4];
    n2 missileStretch[4];

    n8 pendingPosition[8];
    n3 positionDelay[8];
    n8 pendingGraphics[5];
    n2 graphicsDelay[5];
    n2 pendingPlayerSize[4];
    n3 playerSizeDelay[4];
    n8 pendingMissileSize;
    n3 missileSizeDelay;
  } playerMissile;

  struct Priority {
    struct Rule {
      bool pri0;
      bool pri2;
      bool pri01;
      bool pri12;
      bool pri23;
      bool pri03;
    };

    struct Signals {
      u8 players;
      u8 playfields;
      bool background;
    };

    static constexpr Rule Rules[16] = {
      {0, 0, 0, 0, 0, 0}, {1, 0, 1, 0, 0, 1},
      {0, 0, 1, 1, 0, 0}, {1, 0, 1, 1, 0, 1},
      {0, 1, 0, 1, 1, 0}, {1, 1, 1, 1, 1, 1},
      {0, 1, 1, 1, 1, 0}, {1, 1, 1, 1, 1, 1},
      {0, 0, 0, 0, 1, 1}, {1, 0, 1, 0, 1, 1},
      {0, 0, 1, 1, 1, 1}, {1, 0, 1, 1, 1, 1},
      {0, 1, 0, 1, 1, 1}, {1, 1, 1, 1, 1, 1},
      {0, 1, 1, 1, 1, 1}, {1, 1, 1, 1, 1, 1},
    };

    //priority.cpp
    auto clock() -> void;
    auto write(n8 data) -> void;
    auto signals(u8 players, u8 playfields) const -> Signals;
    auto mode() const -> u8;
    auto rule() const -> u8;
    auto fifthPlayer() const -> bool;
    auto multicolor() const -> bool;

    n8 control;
    n6 pendingLow;
    n2 lowDelay;
    n2 pendingMode;
    n3 modeDelay;
  } priority;

  struct Playfield {
    struct Signals {
      int collision;
      u8 playfields;
      u8 specialPlayers;
    };

    struct Input {
      Sample sample[2];
      n1 bit[2];
      n1 synchronize;
    };

    //playfield.cpp
    auto power() -> void;
    auto clock(n3 an) -> Input;
    auto signals(Sample sample, u8 special, u8 mode) const -> Signals;
    auto clearHighResolution() -> void;

    struct Special {
      Sample sample[4];
      n4 players[4];
      n4 missiles[4];
      n1 bit[4];
      u32 y;
      u32 x;
      n4 mode10Previous;
    } special;
    n1 highResolution;
    n1 horizontalBlank;
  } playfield;

  struct Console {
    //console.cpp
    auto power() -> void;
    auto clock(n3 graphicsControl) -> void;
    auto readTrigger(u8 index, n3 graphicsControl) -> n1;
    auto triggerValue(u8 index) const -> n1;
    auto pins() const -> n4;
    auto releaseTriggers() -> void;
    auto write(n8 data) -> void;

    n4 output;
    n4 pinSense;
    n1 trigger[4];
  } console;

  struct ColorRegisters {
    //color.cpp
    auto power() -> void;
    auto clock() -> void;
    auto write(u8 index, n8 data) -> void;
    auto player(u8 index) const -> n8;
    auto playfield(u8 index) const -> n8;
    auto background() const -> n8;

    n8 playerColor[4];
    n8 playfieldColor[4];
    n8 backgroundColor;
    n8 pendingColor[9];
    n1 colorDelay[9];
  } colors;

  struct Collision {
    //collision.cpp
    auto clock(int playfield, u8 players, u8 missiles) -> void;
    auto read(u8 address) const -> n8;
    auto clear() -> void;

    n4 missilePlayfield[4];
    n4 playerPlayfield[4];
    n4 missilePlayer[4];
    n4 playerPlayer[4];
  } collision;

  struct Counter {
    //timing.cpp
    auto power() -> void;
    auto synchronizeHorizontalBlank() -> void;
    auto advance() -> void;

    n8 horizontal;
    n9 vertical;
  } counter;

  //video.cpp
  auto outputSample(u32 y, u32 x, Sample sample, u8 players, u8 missiles, n1 specialBit) -> void;
  auto resolve(Sample sample, u8 special, u8 players, u8 missiles) -> n8;

  //io.cpp
  auto writeControl(n8 address, n8 data) -> void;

  n3 graphicsControl;
};

extern GTIA gtia;
