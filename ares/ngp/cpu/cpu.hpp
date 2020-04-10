//the Neo Geo Pocket CPU is a custom component, but its internal registers most
//closely match the Toshiba TMP95C061. Until it's known exactly which components
//actually exist, this class tries to emulate all TMP95C061 functionality that
//it is able to.

struct CPU : TLCS900H, Thread {
  Node::Component node;
  ares::Memory::Writable<uint8> ram;  //12KB

  struct Debugger {
    //debugger.cpp
    auto load(Node::Object) -> void;
    auto instruction() -> void;
    auto interrupt(string_view) -> void;

    struct Memory {
      Node::Memory ram;
    } memory;

    struct Tracer {
      Node::Instruction instruction;
      Node::Notification interrupt;
    } tracer;
  } debugger;

  //Neo Geo Pocket Color: 0x87e2 (K2GE mode selection) is a privileged register.
  //the development manual states user-mode code cannot change this value, and
  //yet Dokodemo Mahjong does so anyway, and sets it to grayscale mode despite
  //operating in color mode, and further fails to set the K1GE compatibility
  //palette, resulting in incorrect colors. . I am not certain how, but the NGPC
  //blocks this write command, so I attempt to simulate that here.
  //VPU::write(uint24, uint8) calls this function.
  auto privilegedMode() const -> bool {
    return r.pc.l.l0 >= 0xff0000;  //may also be r.rfp == 3
  }

  //cpu.cpp
  auto load(Node::Object) -> void;
  auto save() -> void;
  auto unload() -> void;

  auto main() -> void;
  auto step(uint clocks) -> void;
  auto idle(uint clocks) -> void override;
  auto pollPowerButton() -> void;
  auto power() -> void;
  auto fastBoot() -> void;

  //memory.cpp
  auto width(uint24 address) -> uint override;
  auto read(uint width, uint24 address) -> uint32 override;
  auto write(uint width, uint24 address, uint32 data) -> void override;

  //io.cpp
  auto readIO(uint8 address) -> uint8;
  auto writeIO(uint8 address, uint8 data) -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

//private:
  struct Interrupts {
    //interrupts.cpp
    auto poll() -> void;
    auto fire() -> bool;

    uint8 vector;
    uint3 priority;
  } interrupts;

  struct Interrupt {
    //interrupts.cpp
    auto power(uint8 vector) -> void;
    auto operator=(bool) -> void;
    auto poll(uint8& vector, uint3& priority) -> void;
    auto fire(uint8 vector) -> void;
    auto set(bool line) -> void;
    auto raise() -> void;
    auto lower() -> void;
    auto trigger() -> void;
    auto clear() -> void;

    auto setEnable(uint1 enable) -> void;
    auto setPriority(uint3 priority) -> void;

    //serialization.cpp
    auto serialize(serializer&) -> void;

    uint8 vector;
    uint1 dmaAllowed;
    uint1 enable;
    uint1 maskable;
    uint3 priority;
    uint1 line;
    uint1 pending;

    struct Level {
      uint1 high;
      uint1 low;
    } level;

    struct Edge {
      uint1 rising;
      uint1 falling;
    } edge;
  };

  //non-maskable
  Interrupt nmi;
  Interrupt intwd;

  //maskable
  Interrupt int0;
  Interrupt int4;
  Interrupt int5;
  Interrupt int6;
  Interrupt int7;
  Interrupt intt0;
  Interrupt intt1;
  Interrupt intt2;
  Interrupt intt3;
  Interrupt inttr4;
  Interrupt inttr5;
  Interrupt inttr6;
  Interrupt inttr7;
  Interrupt intrx0;
  Interrupt inttx0;
  Interrupt intrx1;
  Interrupt inttx1;
  Interrupt intad;
  Interrupt inttc0;
  Interrupt inttc1;
  Interrupt inttc2;
  Interrupt inttc3;

  struct DMA {
    uint8 vector;
  } dma0, dma1, dma2, dma3;

  //ports.cpp
  struct PortFlow {
    auto readable() const { return flow == 0; }  //in
    auto writable() const { return flow == 1; }  //out
    uint1 flow;
  };

  struct PortMode {
    auto internal() const { return mode == 0; }  //port
    auto external() const { return mode == 1; }  //timer, etc
    uint1 mode;
  };

  // P10-17, D8-D15
  struct P1 : PortFlow {
    operator bool() const;
    auto operator=(bool) -> void;
    uint1 latch;
  } p10, p11, p12, p13, p14, p15, p16, p17;

  // P20-P27, A16-A23
  struct P2 : PortMode {
    operator bool() const;
    auto operator=(bool) -> void;
    uint1 latch;
  } p20, p21, p22, p23, p24, p25, p26, p27;

  // P52, /HWR
  // P53, /BUSRQ
  // P54, /BUSAK
  // P55, R, /W
  struct P5 : PortFlow, PortMode {
    operator bool() const;
    auto operator=(bool) -> void;
    uint1 latch;
  } p52, p53, p54, p55;

  // P60, /CS0
  // P61, /CS1
  // P62, /CS2
  // P63, /CS3, /CAS
  // P64, /RAS
  // P65, /REFOUT
  struct P6 : PortMode {
    operator bool() const;
    auto operator=(bool) -> void;
    uint1 latch;
  } p60, p61, p62, p63, p64, p65;

  // P70-P73, PG00-PG03
  // P74-P77, PG10-PG13
  struct P7 : PortFlow, PortMode {
    operator bool() const;
    auto operator=(bool) -> void;
    uint1 latch;
  } p70, p71, p72, p73, p74, p75, p76, p77;

  // P80, TXD0
  // P81, RXD0
  // P82, /CTS0, SCLK0
  // P83, TCD1
  // P84, RXD1
  // P85, SCLK1
  struct P8F : PortFlow {
    operator bool() const;
    auto operator=(bool) -> void;
    uint1 latch;
  } p81, p84;

  struct P8M : PortFlow, PortMode {
    operator bool() const;
    auto operator=(bool) -> void;
    uint1 latch;
  } p80, p82, p83, p85;

  // P90-P93, AN0-AN3
  struct P9 {
    operator bool() const;
    uint1 latch;
  } p90, p91, p92, p93;

  // PA0, /WAIT
  struct PA0 : PortFlow {
    operator bool() const;
    auto operator=(bool) -> void;
    uint1 latch;
  } pa0;

  // PA1, TI0, (/HBLANK)
  struct PA1 : PortFlow {
    operator bool() const;
    auto operator=(bool) -> void;
    uint1 latch;
  } pa1;

  // PA2, TO1
  struct PA2 : PortFlow, PortMode {
    operator bool() const;
    auto operator=(bool) -> void;
    uint1 latch;
  } pa2;

  // PA3, TO3
  struct PA3 : PortFlow, PortMode {
    operator bool() const;
    auto operator=(bool) -> void;
    uint1 latch;
  } pa3;

  // PB0, INT4, TI4, (/VBLANK)
  struct PB0 : PortFlow {
    operator bool() const;
    auto operator=(bool) -> void;
    uint1 latch;
  } pb0;

  // PB1, INT5, TI5, (APU)
  struct PB1 : PortFlow {
    operator bool() const;
    auto operator=(bool) -> void;
    uint1 latch;
  } pb1;

  // PB2, TO4
  struct PB2 : PortFlow, PortMode {
    operator bool() const;
    auto operator=(bool) -> void;
    uint1 latch;
  } pb2;

  // PB3, TO5
  struct PB3 : PortFlow, PortMode {
    operator bool() const;
    auto operator=(bool) -> void;
    uint1 latch;
  } pb3;

  // PB4, INT6, TI6
  struct PB4 : PortFlow {
    operator bool() const;
    auto operator=(bool) -> void;
    uint1 latch;
  } pb4;

  // PB5, INT7, TI7
  struct PB5 : PortFlow {
    operator bool() const;
    auto operator=(bool) -> void;
    uint1 latch;
  } pb5;

  // PB6, TO6
  struct PB6 : PortFlow, PortMode {
    operator bool() const;
    auto operator=(bool) -> void;
    uint1 latch;
  } pb6;

  // PB7, INT0
  struct PB7 : PortFlow {
    operator bool() const;
    auto operator=(bool) -> void;
    uint1 latch;
  } pb7;

  //timers.cpp
  struct Prescaler {
    auto step(uint clocks) -> void;

    uint1  enable;
    uint32 counter;
  } prescaler;

  struct TI0 {
    operator bool() const { return latch; }
    auto operator=(bool) -> void;
    uint1 latch;
  } ti0;

  struct TI4 {
    operator bool() const { return latch; }
    auto operator=(bool) -> void;
    uint1 latch;
  } ti4;

  struct TI5 {
    operator bool() const { return latch; }
    auto operator=(bool) -> void;
    uint1 latch;
  } ti5;

  struct TI6 {
    operator bool() const { return latch; }
    auto operator=(bool) -> void;
    uint1 latch;
  } ti6;

  struct TI7 {
    operator bool() const { return latch; }
    auto operator=(bool) -> void;
    uint1 latch;
  } ti7;

  struct TO1 {
    operator bool() const { return latch; }
    auto operator=(bool) -> void;
    uint1 latch;
  } to1;

  struct TO3 {
    operator bool() const { return latch; }
    auto operator=(bool) -> void;
    uint1 latch;
  } to3;

  struct TO4 {
    operator bool() const { return latch; }
    auto operator=(bool) -> void;
    uint1 latch;
  } to4;

  struct TO5 {
    operator bool() const { return latch; }
    auto operator=(bool) -> void;
    uint1 latch;
  } to5;

  struct TO6 {
    operator bool() const { return latch; }
    auto operator=(bool) -> void;
    uint1 latch;
  } to6;

  struct TO7 {
    operator bool() const { return latch; }
    auto operator=(bool) -> void;
    uint1 latch;
  } to7;

  //8-bit timers

  struct Timer0 {
    auto disable() -> void;

    uint1 enable;   //t0run
    uint2 mode;     //t0clk
    uint8 counter;  //uc0
    uint8 compare;  //treg0
  } t0;

  struct Timer1 {
    auto disable() -> void;

    uint1 enable;   //t1run
    uint2 mode;     //t1clk
    uint8 counter;  //uc1
    uint8 compare;  //treg1
  } t1;

  struct FlipFlop1 {
    operator bool() const { return output; }
    auto operator=(bool) -> void;

    uint1 source;  //ff1is
    uint1 invert;  //ff1ie
    uint1 output;
  } ff1;

  struct Timer01 {
    auto clockT0() -> void;
    auto clockT1() -> void;

    uint2 mode;  //t01m
    uint2 pwm;   //pwm0
    struct Buffer {
      uint1 enable;
      uint8 compare;
    } buffer;
  } t01;

  struct Timer2 {
    auto disable() -> void;

    uint1 enable;
    uint2 mode;
    uint8 counter;
    uint8 compare;
  } t2;

  struct Timer3 {
    auto disable() -> void;

    uint1 enable;
    uint2 mode;
    uint8 counter;
    uint8 compare;
  } t3;

  struct FlipFlop3 {
    operator bool() const { return output; }
    auto operator=(bool) -> void;

    uint1 source;
    uint1 invert;
    uint1 output;
  } ff3;

  struct Timer23 {
    auto clockT2() -> void;
    auto clockT3() -> void;

    uint2 mode;
    uint2 pwm;
    struct Buffer {
      uint1 enable;
      uint8 compare;
    } buffer;
  } t23;

  //16-bit timers

  struct FlipFlop4 {
    operator bool() const { return output; }
    auto operator=(bool) -> void;

    uint1 flipOnCompare4;
    uint1 flipOnCompare5;
    uint1 flipOnCapture1;
    uint1 flipOnCapture2;
    uint1 output;
  } ff4;

  struct FlipFlop5 {
    operator bool() const { return output; }
    auto operator=(bool) -> void;

    uint1 flipOnCompare5;
    uint1 flipOnCapture2;
    uint1 output;
  } ff5;

  struct Timer4 {
    auto disable() -> void;
    auto captureTo1() -> void;
    auto captureTo2() -> void;
    auto clock() -> void;

    uint1  enable;
    uint2  mode;
    uint2  captureMode;
    uint1  clearOnCompare5;
    uint16 counter;
    uint16 compare4;
    uint16 compare5;
    uint16 capture1;
    uint16 capture2;
    struct Buffer {
      uint1  enable;
      uint16 compare;
    } buffer;
  } t4;

  struct FlipFlop6 {
    operator bool() const { return output; }
    auto operator=(bool) -> void;

    uint1 flipOnCompare6;
    uint1 flipOnCompare7;
    uint1 flipOnCapture3;
    uint1 flipOnCapture4;
    uint1 output;
  } ff6;

  struct Timer5 {
    auto disable() -> void;
    auto captureTo3() -> void;
    auto captureTo4() -> void;
    auto clock() -> void;

    uint1  enable;
    uint2  mode;
    uint2  captureMode;
    uint1  clearOnCompare7;
    uint16 counter;
    uint16 compare6;
    uint16 compare7;
    uint16 capture3;
    uint16 capture4;
    struct Buffer {
      uint1  enable;
      uint16 compare;
    } buffer;
  } t5;

  //adc.cpp
  struct ADC {
    auto step(uint clocks) -> void;

    uint32 counter;
    uint2  channel;
    uint1  speed;  //0 = 160 states, 1 = 320 states
    uint1  scan;
    uint1  repeat;
    uint1  busy;
    uint1  end;
    uint10 result[4];
  } adc;

  //rtc.cpp
  struct RTC {
    auto step(uint clocks) -> void;
    auto daysInMonth() -> uint8;
    auto daysInFebruary() -> uint8;

    uint32 counter;
    uint1  enable;
    uint8  second;
    uint8  minute;
    uint8  hour;
    uint8  weekday;
    uint8  day;
    uint8  month;
    uint8  year;
  } rtc;

  //watchdog.cpp
  struct Watchdog {
    auto step(uint clocks) -> void;

    uint32 counter;
    uint1  enable;
    uint1  drive;
    uint1  reset;
    uint2  standby;
    uint1  warmup;
    uint2  frequency;
  } watchdog;

  //memory.cpp
  struct Bus {
    auto wait() -> void;
    auto read(uint width, uint24 address) -> uint32;
    auto write(uint width, uint24 address, uint32 data) -> void;

    uint8 width;
    uint2 timing;
    function<uint8 (uint24)> reader;
    function<void (uint24, uint8)> writer;
  };

  struct IO : Bus {
    auto select(uint24 address) const -> bool;
  } io;

  struct ROM : Bus {
    auto select(uint24 address) const -> bool;
  } rom;

  struct CRAM : Bus {
    auto select(uint24 address) const -> bool;
  } cram;

  struct ARAM : Bus {
    auto select(uint24 address) const -> bool;
  } aram;

  struct VRAM : Bus {
    auto select(uint24 address) const -> bool;
  } vram;

  struct CS0 : Bus {
    auto select(uint24 address) const -> bool;

     uint1 enable;
    uint24 address;
    uint24 mask;
  } cs0;

  struct CS1 : Bus {
    auto select(uint24 address) const -> bool;

     uint1 enable;
    uint24 address;
    uint24 mask;
  } cs1;

  struct CS2 : Bus {
    auto select(uint24 address) const -> bool;

     uint1 enable;
    uint24 address;
    uint24 mask;
     uint1 mode;  //0 = fixed; 1 = address/mask
  } cs2;

  struct CS3 : Bus {
    auto select(uint24 address) const -> bool;

     uint1 enable;
    uint24 address;
    uint24 mask;
     uint1 cas;  //0 = /CS3, 1 = /CAS
  } cs3;

  struct CSX : Bus {
  } csx;

  struct Clock {
    //0 = 6144000hz
    //1 = 3072000hz
    //2 = 1536000hz
    //3 =  768000hz
    //4 =  384000hz
    //5 =  192000hz? (undocumented)
    //6 =   96000hz? (undocumented)
    //7 =   48000hz? (undocumented)
    uint3 rate = 4;  //default value unconfirmed
  } clock;

  struct Misc {
    uint1 p5;  //Port 5: 0 = PSRAM mode, 1 = (not PSRAM mode?) [unemulated]
    uint1 rtsDisable;
    uint8 b4;
    uint8 b5;
  } misc;
};

extern CPU cpu;
