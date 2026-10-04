struct ANTIC : Thread {
  Node::Object node;

  //antic.cpp
  auto load(Node::Object parent) -> void;
  auto unload() -> void;
  auto main() -> void;
  auto power() -> void;

  //io.cpp
  auto read(n8 address) -> n8;
  auto peek(n8 address) const -> n8;
  auto write(n8 address, n8 data) -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

//private:

  struct ModeProperties {
    u8 height;
    u8 bytes;
    u8 bitsPerPixel;
    u8 samplesPerPixel;
    bool character;
    bool highResolution;
  };

  static constexpr ModeProperties Modes[16] = {
    { 1,  0, 0, 0, false, false},
    { 1,  0, 0, 0, false, false},
    { 8, 40, 1, 1, true,  true },
    {10, 40, 1, 1, true,  true },
    { 8, 40, 2, 2, true,  false},
    {16, 40, 2, 2, true,  false},
    { 8, 20, 1, 2, true,  false},
    {16, 20, 1, 2, true,  false},
    { 8, 10, 2, 8, false, false},
    { 4, 10, 1, 4, false, false},
    { 4, 20, 2, 4, false, false},
    { 2, 20, 1, 2, false, false},
    { 1, 20, 1, 2, false, false},
    { 2, 40, 2, 2, false, false},
    { 1, 40, 2, 2, false, false},
    { 1, 40, 1, 1, false, true },
  };

  enum class DMAConsumer : u8 {
    Missile,
    Player,
    DisplayListInstruction,
    DisplayListOperand,
    LineBufferName,
    LineBufferBitmap,
    Character,
  };

  //timing.cpp
  auto scanline() -> void;
  auto clock() -> void;
  auto step(u32 colorClocks) -> void;
  auto clockRegisterPipelines() -> void;
  auto finishScanline() -> void;
  auto frame() -> void;

  struct IO {
    n6 dmactl;
    n3 chactl;
    n16 dlist;
    n4 hscroll;
    n4 vscroll;
    n8 pmbase;
    n8 chbase;
    n8 penh;
    n8 penv;
    n8 nmien;
    n8 nmist;
    n8 chbasePipeline[2];
    n2 playerMissileDMA[2];
  } io;

  struct Counter {
    n8 machineCycle;
    n9 scanline;
  } counter;

  struct DMA {
    ANTIC& self;
    DMA(ANTIC& self) : self(self) {}

    struct Request {
      n16 address;
      DMAConsumer consumer;
      n8 index;
      n8 phase;
      n1 addressValid;
      n1 physical;
      n1 next;
    };

    struct Requests {
      Request item[4];
      n3 size;
      n1 lineBufferReplay;
      n6 lineBufferPhase;

      //dma.cpp
      auto append(n16 address, DMAConsumer consumer, u8 index,
        bool addressValid, bool physical, bool next = false, u8 phase = 0) -> void;
      auto replayLineBuffer(n6 phase) -> void;
    };

    //dma.cpp
    auto power() -> void;
    auto beginScanline() -> void;
    auto clockRefresh() -> void;
    auto playerMissileAddress(u8 index) const -> n16;
    auto queuePlayerMissile(Requests& requests) -> void;
    auto arbitrate(Requests& requests) -> void;
    auto consume(const Request& request, n8 data) -> void;

    //serialization.cpp
    auto serialize(serializer&) -> void;

    n1 refreshPending;
  } dma{*this};

  struct DisplayList {
    ANTIC& self;
    DisplayList(ANTIC& self) : self(self) {}

    //display-list.cpp
    auto power() -> void;
    auto incrementDisplayList(n16 address) const -> n16;
    auto incrementMemoryScan(n16 address) const -> n16;
    auto beginInstruction() -> void;
    auto beginScanline() -> void;
    auto finishScanline() -> void;
    auto clock() -> void;
    auto queueDMA(DMA::Requests& requests) -> void;
    auto consumeInstruction(n8 data) -> void;
    auto consumeOperand(u8 index, n8 data) -> void;

    //serialization.cpp
    auto serialize(serializer&) -> void;

    n8 instruction;
    n16 memoryScan;
    n8 row;
    n8 lastRow;
    n1 valid;
    n1 needInstruction;
    n1 waitingForVerticalBlank;
    n1 loadMemoryScan;
    n1 verticalScroll;
    n1 verticalScrollEnding;
    n1 firstScanline;
    n1 dmaEnabled;
    n2 operand;
    n16 lineAddress;
    n4 vscrollStart;
    n4 vscrollDLI;
    n4 vscrollEnd;
  } displayList{*this};

  struct Playfield {
    ANTIC& self;
    Playfield(ANTIC& self) : self(self) {}

    //playfield.cpp
    auto power() -> void;
    auto start() const -> u16;
    auto end() const -> u16;
    auto fetchWidth() const -> u8;
    auto sample() const -> n3;
    auto clockAN(u16 colorClock) -> n3;
    auto characterAddress(n8 name, u8 row) const -> n16;
    auto lineBufferRead(n6 phase) const -> n8;
    auto lineBufferWrite(n6 phase, n8 data) -> void;
    auto bitmapLineBufferPhase(bool next) -> n6;
    auto incrementLineBuffer(n6& phase) -> void;
    auto dmaFeedbackLength() const -> u8;
    auto dmaStart() const -> u8;
    auto dmaStop() const -> u8;
    auto clearDMARing() -> void;
    auto clockDMARing() -> void;
    auto scheduleGraphics(n8 data, n8 name, u8 delay) -> void;
    auto scheduleLineGraphics(n6 phase, u8 delay) -> void;
    auto clockShiftRing() -> void;
    auto queueDMA(DMA::Requests& requests) -> void;
    auto consumeName(n6 writePhase, n6 readPhase, n8 data) -> void;
    auto consumeBitmap(n6 writePhase, n6 readPhase, n8 data) -> void;
    auto consumeCharacter(n8 character, n8 data) -> void;

    //serialization.cpp
    auto serialize(serializer&) -> void;

    n8 dmaClock;
    n8 dmaName[8];
    n6 lineWrite;
    n6 lineRead;
    n8 lineBuffer[48];

    n4 shiftClock;
    n8 graphics;
    n8 name;
    n2 output;
    n3 delayed;
    n4 queuePosition;
    n16 queueValid;
    n16 queueLine;
    n8 queueData[16];
    n8 queueName[16];
    n6 queuePhase[16];
  } playfield{*this};

  struct WSYNC {
    ANTIC& self;
    WSYNC(ANTIC& self) : self(self) {}

    //interrupt.cpp
    auto power() -> void;
    auto clock() -> void;
    auto wait() -> void;

    //serialization.cpp
    auto serialize(serializer&) -> void;

    n1 active;
    n9 scanline;
  } wsync{*this};

  struct Interrupt {
    ANTIC& self;
    Interrupt(ANTIC& self) : self(self) {}

    //interrupt.cpp
    auto power() -> void;
    auto schedule(n8 source) -> void;
    auto clock() -> void;

    //serialization.cpp
    auto serialize(serializer&) -> void;

    n8 pending;
    n8 enable;
    n1 pulse;
  } interrupt{*this};
};

extern ANTIC antic;
