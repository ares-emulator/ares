//the Neo Geo Pocket CPU is a custom component, but its internal registers most
//closely match the Toshiba TMP95C061. Until it's known exactly which components
//actually exist, this class tries to emulate all TMP95C061 functionality that
//it is able to.

struct CPU : TLCS900H, Thread {
  Node::Object node;
  ares::Memory::Writable<n8> ram;  //12KB

  struct Debugger {
    CPU& self;

    //debugger.cpp
    auto load(Node::Object) -> void;
    auto unload(Node::Object) -> void;
    auto instruction() -> void;
    auto interrupt(u8 vector) -> void;
    auto readIO(u8 address, u8 data) -> void;
    auto writeIO(u8 address, u8 data) -> void;

    struct Memory {
      Node::Debugger::Memory ram;
    } memory;

    struct Tracer {
      Node::Debugger::Tracer::Instruction instruction;
      Node::Debugger::Tracer::Notification interrupt;
      Node::Debugger::Tracer::Notification systemCall;
      Node::Debugger::Tracer::Notification io;
    } tracer;

    vector<n24> vectors;
  } debugger{*this};

  //Neo Geo Pocket Color: 0x87e2 (K2GE mode selection) is a privileged register.
  //the development manual states user-mode code cannot change this value, and
  //yet Dokodemo Mahjong does so anyway, and sets it to grayscale mode despite
  //operating in color mode, and further fails to set the K1GE compatibility
  //palette, resulting in incorrect colors. I am not certain how, but the NGPC
  //blocks this write command, so I attempt to simulate that here.
  //KGE::write(n24, n8) calls this function.
  auto privilegedMode() const -> bool {
    return TLCS900H::load(PC) >= 0xff0000;  //may also be RFP == 3
  }

  //cpu.cpp
  auto load(Node::Object) -> void;
  auto save() -> void;
  auto unload() -> void;

  auto main() -> void;
  auto step(u32 clocks) -> void override;
  auto pollPowerButton() -> void;
  auto power() -> void;

  //memory.cpp
  auto width(n24 address) -> u32 override;
  auto speed(u32 width, n24 address) -> n32 override;
  auto read(u32 width, n24 address) -> n32 override;
  auto write(u32 width, n24 address, n32 data) -> void override;
  auto disassembleRead(n24 address) -> n8 override;

  //io.cpp
  auto readIO(n8 address) -> n8;
  auto writeIO(n8 address, n8 data) -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

//private:
  struct Interrupts {
    //interrupts.cpp
    auto poll() -> void;
    auto fire() -> bool;

    n8 vector;
    n3 priority;
  } interrupts;

  struct Interrupt {
    //interrupts.cpp
    auto power(n8 vector) -> void;
    auto operator=(bool) -> void;
    auto poll(n8& vector, n3& priority) -> void;
    auto fire(n8 vector) -> void;
    auto set(bool line) -> void;
    auto raise() -> void;
    auto lower() -> void;
    auto trigger() -> void;
    auto clear() -> void;

    auto setEnable(n1 enable) -> void;
    auto setPriority(n3 priority) -> void;

    //serialization.cpp
    auto serialize(serializer&) -> void;

    n8 vector;
    n1 dmaAllowed;
    n1 enable;
    n1 maskable;
    n3 priority;
    n1 line;
    n1 pending;

    struct Level {
      n1 high;
      n1 low;
    } level;

    struct Edge {
      n1 rising;
      n1 falling;
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
    n8 vector;
  } dma0, dma1, dma2, dma3;

  //ports.cpp
  struct PortFlow {
    auto readable() const { return flow == 0; }  //in
    auto writable() const { return flow == 1; }  //out
    n1 flow;
  };

  struct PortMode {
    auto internal() const { return mode == 0; }  //port
    auto external() const { return mode == 1; }  //timer, etc
    n1 mode;
  };

  // P10-17, D8-D15
  struct P1 : PortFlow {
    operator bool() const;
    auto operator=(bool) -> void;
    n1 latch;
  } p10, p11, p12, p13, p14, p15, p16, p17;

  // P20-P27, A16-A23
  struct P2 : PortMode {
    operator bool() const;
    auto operator=(bool) -> void;
    n1 latch;
  } p20, p21, p22, p23, p24, p25, p26, p27;

  // P52, /HWR
  // P53, /BUSRQ
  // P54, /BUSAK
  // P55, R, /W
  struct P5 : PortFlow, PortMode {
    operator bool() const;
    auto operator=(bool) -> void;
    n1 latch;
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
    n1 latch;
  } p60, p61, p62, p63, p64, p65;

  // P70-P73, PG00-PG03
  // P74-P77, PG10-PG13
  struct P7 : PortFlow, PortMode {
    operator bool() const;
    auto operator=(bool) -> void;
    n1 latch;
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
    n1 latch;
  } p81, p84;

  struct P8MA : PortFlow, PortMode {
    operator bool() const;
    auto operator=(bool) -> void;
    n1 latch;
    n1 drain;  //0 = CMOS output, 1 = open-drain output
  } p80, p83;

  struct P8MB : PortFlow, PortMode {
    operator bool() const;
    auto operator=(bool) -> void;
    n1 latch;
  } p82, p85;

  // P90-P93, AN0-AN3
  struct P9 {
    operator bool() const;
    n1 latch;
  } p90, p91, p92, p93;

  // PA0, /WAIT
  struct PA0 : PortFlow {
    operator bool() const;
    auto operator=(bool) -> void;
    n1 latch;
  } pa0;

  // PA1, TI0, (/HBLANK)
  struct PA1 : PortFlow {
    operator bool() const;
    auto operator=(bool) -> void;
    n1 latch;
  } pa1;

  // PA2, TO1
  struct PA2 : PortFlow, PortMode {
    operator bool() const;
    auto operator=(bool) -> void;
    n1 latch;
  } pa2;

  // PA3, TO3
  struct PA3 : PortFlow, PortMode {
    operator bool() const;
    auto operator=(bool) -> void;
    n1 latch;
  } pa3;

  // PB0, INT4, TI4, (/VBLANK)
  struct PB0 : PortFlow {
    operator bool() const;
    auto operator=(bool) -> void;
    n1 latch;
  } pb0;

  // PB1, INT5, TI5, (APU)
  struct PB1 : PortFlow {
    operator bool() const;
    auto operator=(bool) -> void;
    n1 latch;
  } pb1;

  // PB2, TO4
  struct PB2 : PortFlow, PortMode {
    operator bool() const;
    auto operator=(bool) -> void;
    n1 latch;
  } pb2;

  // PB3, TO5
  struct PB3 : PortFlow, PortMode {
    operator bool() const;
    auto operator=(bool) -> void;
    n1 latch;
  } pb3;

  // PB4, INT6, TI6
  struct PB4 : PortFlow {
    operator bool() const;
    auto operator=(bool) -> void;
    n1 latch;
  } pb4;

  // PB5, INT7, TI7
  struct PB5 : PortFlow {
    operator bool() const;
    auto operator=(bool) -> void;
    n1 latch;
  } pb5;

  // PB6, TO6
  struct PB6 : PortFlow, PortMode {
    operator bool() const;
    auto operator=(bool) -> void;
    n1 latch;
  } pb6;

  // PB7, INT0
  struct PB7 : PortFlow {
    operator bool() const;
    auto operator=(bool) -> void;
    n1 latch;
  } pb7;

  //timers.cpp
  struct Prescaler {
    auto step(u32 clocks) -> void;

    n1  enable;
    n32 counter;
  } prescaler;

  struct TI0 {
    operator bool() const { return latch; }
    auto operator=(bool) -> void;
    n1 latch;
  } ti0;

  struct TI4 {
    operator bool() const { return latch; }
    auto operator=(bool) -> void;
    n1 latch;
  } ti4;

  struct TI5 {
    operator bool() const { return latch; }
    auto operator=(bool) -> void;
    n1 latch;
  } ti5;

  struct TI6 {
    operator bool() const { return latch; }
    auto operator=(bool) -> void;
    n1 latch;
  } ti6;

  struct TI7 {
    operator bool() const { return latch; }
    auto operator=(bool) -> void;
    n1 latch;
  } ti7;

  struct TO1 {
    operator bool() const { return latch; }
    auto operator=(bool) -> void;
    n1 latch;
  } to1;

  struct TO3 {
    operator bool() const { return latch; }
    auto operator=(bool) -> void;
    n1 latch;
  } to3;

  struct TO4 {
    operator bool() const { return latch; }
    auto operator=(bool) -> void;
    n1 latch;
  } to4;

  struct TO5 {
    operator bool() const { return latch; }
    auto operator=(bool) -> void;
    n1 latch;
  } to5;

  struct TO6 {
    operator bool() const { return latch; }
    auto operator=(bool) -> void;
    n1 latch;
  } to6;

  struct TO7 {
    operator bool() const { return latch; }
    auto operator=(bool) -> void;
    n1 latch;
  } to7;

  //8-bit timers

  struct Timer0 {
    auto disable() -> void;

    n1 enable;   //t0run
    n2 mode;     //t0clk
    n8 counter;  //uc0
    n8 compare;  //treg0
  } t0;

  struct Timer1 {
    auto disable() -> void;

    n1 enable;   //t1run
    n2 mode;     //t1clk
    n8 counter;  //uc1
    n8 compare;  //treg1
  } t1;

  struct FlipFlop1 {
    operator bool() const { return output; }
    auto operator=(bool) -> void;

    n1 source;  //ff1is
    n1 invert;  //ff1ie
    n1 output;
  } ff1;

  struct Timer01 {
    auto clockT0() -> void;
    auto clockT1() -> void;

    n2 mode;  //t01m
    n2 pwm;   //pwm0
    struct Buffer {
      n1 enable;
      n8 compare;
    } buffer;
  } t01;

  struct Timer2 {
    auto disable() -> void;

    n1 enable;
    n2 mode;
    n8 counter;
    n8 compare;
  } t2;

  struct Timer3 {
    auto disable() -> void;

    n1 enable;
    n2 mode;
    n8 counter;
    n8 compare;
  } t3;

  struct FlipFlop3 {
    operator bool() const { return output; }
    auto operator=(bool) -> void;

    n1 source;
    n1 invert;
    n1 output;
  } ff3;

  struct Timer23 {
    auto clockT2() -> void;
    auto clockT3() -> void;

    n2 mode;
    n2 pwm;
    struct Buffer {
      n1 enable;
      n8 compare;
    } buffer;
  } t23;

  //16-bit timers

  struct FlipFlop4 {
    operator bool() const { return output; }
    auto operator=(bool) -> void;

    n1 flipOnCompare4;
    n1 flipOnCompare5;
    n1 flipOnCapture1;
    n1 flipOnCapture2;
    n1 output;
  } ff4;

  struct FlipFlop5 {
    operator bool() const { return output; }
    auto operator=(bool) -> void;

    n1 flipOnCompare5;
    n1 flipOnCapture2;
    n1 output;
  } ff5;

  struct Timer4 {
    auto disable() -> void;
    auto captureTo1() -> void;
    auto captureTo2() -> void;
    auto clock() -> void;

    n1  enable;
    n2  mode;
    n2  captureMode;
    n1  clearOnCompare5;
    n16 counter;
    n16 compare4;
    n16 compare5;
    n16 capture1;
    n16 capture2;
    struct Buffer {
      n1  enable;
      n16 compare;
    } buffer;
  } t4;

  struct FlipFlop6 {
    operator bool() const { return output; }
    auto operator=(bool) -> void;

    n1 flipOnCompare6;
    n1 flipOnCompare7;
    n1 flipOnCapture3;
    n1 flipOnCapture4;
    n1 output;
  } ff6;

  struct Timer5 {
    auto disable() -> void;
    auto captureTo3() -> void;
    auto captureTo4() -> void;
    auto clock() -> void;

    n1  enable;
    n2  mode;
    n2  captureMode;
    n1  clearOnCompare7;
    n16 counter;
    n16 compare6;
    n16 compare7;
    n16 capture3;
    n16 capture4;
    struct Buffer {
      n1  enable;
      n16 compare;
    } buffer;
  } t5;

  struct PatternGenerator {
    n1 shiftTrigger;  //0 = 8-bit (timer 0,1 or 2,3), 1 = 16-bit (timer 4 or 5)

    n4 shiftAlternateRegister;
    n4 patternGenerationOutput;

    n1 triggerInputEnable;
    n1 excitationMode;     //0 = 1 or 2 (full step), 1 = 1-2 (half step)
    n1 rotatingDirection;  //0 = normal rotation, 1 = reverse rotation
    n1 writeMode;          //0 = 8-bit, 1 = 4-bit
  } pg0, pg1;

  //serial.cpp
  struct SerialChannel {
    auto receive() -> n8;
    auto transmit(n8 data) -> void;

    n8 buffer;

    n4 baudRateDividend;  //0 = 16, 1 = invalid, 2-15 = 2-15
    n2 baudRateDivider;   //0 = T0, 1 = T2, 2 = T8, 3 = T32

    n1 inputClock;      //0 = baud rate generator, 1 = SCLK pin input
    n1 clockEdge;       //0 = rising edge, 1 = falling edge
    n1 framingError;
    n1 parityError;
    n1 overrunError;
    n1 parityAddition;  //1 = enable
    n1 parity;          //0 = odd, 1 = even
    n1 receiveBit8;

    n2 clock;           //0 = TO2, 1 = baud rate generator, 2 = internal clock, 3 = don't care
    n2 mode;            //0 = I/O interface, 1 = 7-bit UART, 2 = 8-bit, 3 = 9-bit
    n1 wakeUp;          //1 = enable
    n1 receiving;       //1 = enable
    n1 handshake;       //0 = CTS disable, 1 = CTS enable
    n1 transferBit8;    //0 = no, 1 = yes
  } sc0, sc1;

  //adc.cpp
  struct ADC {
    auto step(u32 clocks) -> void;

    n32 counter;
    n2  channel;
    n1  speed;  //0 = 160 states, 1 = 320 states
    n1  scan;
    n1  repeat;
    n1  busy;
    n1  end;
    n10 result[4];
  } adc;

  //rtc.cpp
  struct RTC {
    auto step(u32 clocks) -> void;
    auto daysInMonth() -> n8;
    auto daysInFebruary() -> n8;

    n32 counter;
    n1  enable;
    n8  second;
    n8  minute;
    n8  hour;
    n8  weekday;
    n8  day;
    n8  month;
    n8  year;
  } rtc;

  //watchdog.cpp
  struct Watchdog {
    auto step(u32 clocks) -> void;

    n32 counter;
    n1  enable;
    n1  drive;
    n1  reset;
    n2  standby;
    n1  warmup;
    n2  frequency;
  } watchdog;

  struct DRAM {
    n1 refreshCycle;           //0 = not inserted, 1 = inserted
    n3 refreshCycleWidth;      //2-9 states
    n3 refreshCycleInsertion;  //31,62,78,97,109,124,154,195 states
    n1 dummyCycle;             //0 = prohibit, 1 = execute

    n1 memoryAccessEnable;      //1 = enable
    n2 multiplexAddressLength;  //8-11 bits
    n1 multiplexAddressEnable;  //1 = enable
    n1 memoryAccessSpeed;       //0 = normal, 1 = slow
    n1 busReleaseMode;          //DRAM control signals: 0 = also released, 1 = not released
    n1 selfRefresh = 1;         //0 = execute, 1 = release
  } dram;

  //memory.cpp
  struct Bus {
    auto wait() -> void;
    auto speed(u32 width, n24 address) -> n32;
    auto read(u32 width, n24 address) -> n32;
    auto write(u32 width, n24 address, n32 data) -> void;

    n8 width;
    n2 timing;
    function<n8   (n24)> reader;
    function<void (n24, n8)> writer;

  //unserialized:
    n1 debugging;
  };

  struct IO : Bus {
    auto select(n24 address) const -> bool;
  } io;

  struct ROM : Bus {
    auto select(n24 address) const -> bool;
  } rom;

  struct CRAM : Bus {
    auto select(n24 address) const -> bool;
  } cram;

  struct ARAM : Bus {
    auto select(n24 address) const -> bool;
  } aram;

  struct VRAM : Bus {
    auto select(n24 address) const -> bool;
  } vram;

  struct CS0 : Bus {
    auto select(n24 address) const -> bool;

    n1  enable;
    n24 address;
    n24 mask;
  } cs0;

  struct CS1 : Bus {
    auto select(n24 address) const -> bool;

    n1  enable;
    n24 address;
    n24 mask;
  } cs1;

  struct CS2 : Bus {
    auto select(n24 address) const -> bool;

    n1  enable;
    n24 address;
    n24 mask;
    n1  mode;  //0 = fixed; 1 = address/mask
  } cs2;

  struct CS3 : Bus {
    auto select(n24 address) const -> bool;

    n1  enable;
    n24 address;
    n24 mask;
    n1  cas;  //0 = /CS3, 1 = /CAS
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
    n3 rate = 4;  //default value unconfirmed
  } clock;

  struct Misc {
    n1 p5;  //Port 5: 0 = PSRAM mode, 1 = (not PSRAM mode?) [unemulated]
    n1 rtsDisable;
  } misc;

  struct Unknown {
    n8 b4;
    n8 b5;
    n8 b6;
    n8 b7;
  } unknown;

//unserialized:
  n32 pollPowerButtonSkipCounter;
};

extern CPU cpu;
