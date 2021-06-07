//Mega CD

struct MCD : M68000, Thread {
  Node::Object node;
  Node::Port tray;
  Node::Peripheral disc;
  VFS::Pak pak;
  VFS::File fd;
  Memory::Readable<n16> bios;  //BIOS ROM
  Memory::Writable<n16> pram;  //program RAM
  Memory::Writable<n16> wram;  //work RAM
  Memory::Writable<n8 > bram;  //backup RAM

  struct Debugger {
    //debugger.cpp
    auto load(Node::Object) -> void;
    auto instruction() -> void;
    auto interrupt(string_view) -> void;

    struct Memory {
      Node::Debugger::Memory pram;
      Node::Debugger::Memory wram;
      Node::Debugger::Memory bram;
    } memory;

    struct Tracer {
      Node::Debugger::Tracer::Instruction instruction;
      Node::Debugger::Tracer::Notification interrupt;
    } tracer;
  } debugger;

  auto title() const -> string { return information.title; }

  //mcd.cpp
  auto load(Node::Object) -> void;
  auto unload() -> void;

  auto allocate(Node::Port) -> Node::Peripheral;
  auto connect() -> void;
  auto disconnect() -> void;

  auto save() -> void;
  auto main() -> void;
  auto step(u32 clocks) -> void;
  auto idle(u32 clocks) -> void override;
  auto wait(u32 clocks) -> void override;
  auto power(bool reset) -> void;

  //bus-internal.cpp
  auto read(n1 upper, n1 lower, n24 address, n16 data = 0) -> n16 override;
  auto write(n1 upper, n1 lower, n24 address, n16 data) -> void override;

  //bus-external.cpp
  auto readExternal(n1 upper, n1 lower, n22 address, n16 data) -> n16;
  auto writeExternal(n1 upper, n1 lower, n22 address, n16 data) -> void;

  //io-internal.cpp
  auto readIO(n1 upper, n1 lower, n24 address, n16 data) -> n16;
  auto writeIO(n1 upper, n1 lower, n24 address, n16 data) -> void;

  //io-external.cpp
  auto readExternalIO(n1 upper, n1 lower, n24 address, n16 data) -> n16;
  auto writeExternalIO(n1 upper, n1 lower, n24 address, n16 data) -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  struct Information {
    string title;
  } information;

  struct Counter {
    n16 divider;
    n16 dma;
    f64 pcm = 0.0;
  } counter;

  struct IO {
    n1  run;
    n1  request;
    n1  halt = 1;

    n16 wramLatch;
    n1  wramMode;  //MODE: 0 = 2mbit mode, 1 = 1mbit mode
    n1  wramSwitchRequest;
    n1  wramSwitch;
    n1  wramSelect;
    n2  wramPriority;
    n2  pramBank;
    n8  pramProtect;

    //$000070
    n32 vectorLevel4;
  } io;

  struct LED {
    n1 red;
    n1 green;
  } led;

  struct IRQ {
    //irq.cpp
    auto raise() -> bool;
    auto lower() -> bool;
    static auto synchronize() -> void;

    //serialization.cpp
    auto serialize(serializer&) -> void;

    n1 enable;
    n1 pending;
  };

  struct IRQs {
    n1 pending;

    IRQ reset;
    IRQ subcode;
  } irq;

  struct External {
    IRQ irq;
  } external;

  struct Communcation {
    n8  cfm;
    n8  cfs;
    n16 command[8];
    n16 status [8];
  } communication;

  struct CDC {
    //cdc.cpp
    auto poll() -> void;
    auto clock() -> void;
    auto decode(s32 sector) -> void;
    auto read() -> n8;
    auto write(n8 data) -> void;
    auto power(bool reset) -> void;

    //serialization.cpp
    auto serialize(serializer&) -> void;

    Memory::Writable<n16> ram;

    n4  address;
    n12 stopwatch;

    struct IRQ : MCD::IRQ {
      MCD::IRQ decoder;   //DECEIN + DECI
      MCD::IRQ transfer;  //DTEIEN + DTEI
      MCD::IRQ command;   //CMDIEN + CMDI
    } irq;

    struct Command {
      n8 fifo[8];  //COMIN
      n3 read;
      n3 write;
      n1 empty = 1;
    } command;

    struct Status {
      n8 fifo[8];    //SBOUT
      n3 read;
      n3 write;
      n1 empty = 1;
      n1 enable;     //SOUTEN
      n1 active;     //STEN
      n1 busy;       //STBSY
      n1 wait;       //STWAI
    } status;

    struct Transfer {
      //cdc-transfer.cpp
      auto dma() -> void;
      auto read() -> n16;
      auto start() -> void;
      auto complete() -> void;
      auto stop() -> void;

      //serialization.cpp
      auto serialize(serializer&) -> void;

      n3  destination;
      n19 address;

      n16 source;
      n16 target;
      n16 pointer;
      n12 length;

      n1  enable;     //DOUTEN
      n1  active;     //DTEN
      n1  busy;       //DTBSY
      n1  wait;       //DTWAI
      n1  ready;      //DSR
      n1  completed;  //EDT
    } transfer;

    enum : u32 { Mode1 = 0, Mode2 = 1 };
    enum : u32 { Form1 = 0, Form2 = 1 };
    struct Decoder {
      n1 enable;  //DECEN
      n1 mode;    //MODE
      n1 form;    //FORM
      n1 valid;   //!VALST
    } decoder;

    struct Header {
      n8 minute;
      n8 second;
      n8 frame;
      n8 mode;
    } header;

    struct Subheader {
      n8 file;
      n8 channel;
      n8 submode;
      n8 coding;
    } subheader;

    struct Control {
      n1 head;               //SHDREN: 0 = read header, 1 = read subheader
      n1 mode;               //MODE
      n1 form;               //FORM
      n1 commandBreak;       //CMDBK
      n1 modeByteCheck;      //MBCKRQ
      n1 erasureRequest;     //ERAMRQ
      n1 writeRequest;       //WRRQ
      n1 pCodeCorrection;    //PRQ
      n1 qCodeCorrection;    //QRQ
      n1 autoCorrection;     //AUTOQ
      n1 errorCorrection;    //E01RQ
      n1 edcCorrection;      //EDCRQ
      n1 correctionWrite;    //COWREN
      n1 descramble;         //DSCREN
      n1 syncDetection;      //SYDEN
      n1 syncInterrupt;      //SYIEN
      n1 erasureCorrection;  //ERAMSL
      n1 statusTrigger;      //STENTRG
      n1 statusControl;      //STENCTL
    } control;
  } cdc;

  struct CDD {
    struct Status { enum : u32 {
      Stopped       = 0x0,  //motor disabled
      Playing       = 0x1,  //data or audio playback in progress
      Seeking       = 0x2,  //move to specified time
      Scanning      = 0x3,  //skipping to a specified track
      Paused        = 0x4,  //paused at a specific time
      DoorOpened    = 0x5,  //drive tray is open
      ChecksumError = 0x6,  //invalid communication checksum
      CommandError  = 0x7,  //missing command
      FunctionError = 0x8,  //error during command execution
      ReadingTOC    = 0x9,  //reading table of contents
      Tracking      = 0xa,  //currently skipping tracks
      NoDisc        = 0xb,  //no disc in tray or cannot focus
      LeadOut       = 0xc,  //paused in the lead-out area of the disc
      LeadIn        = 0xd,  //paused in the lead-in area of the disc
      TrayMoving    = 0xe,  //drive tray is moving in response to open/close commands
      Test          = 0xf,  //in test mode
    };};

    struct Command { enum : u32 {
      Idle      = 0x0,  //no operation
      Stop      = 0x1,  //stop motor
      Request   = 0x2,  //change report type
      SeekPlay  = 0x3,  //read ROM data
      SeekPause = 0x4,  //seek to a specified location
      Pause     = 0x6,  //pause the drive
      Play      = 0x7,  //start playing from the current location
      Forward   = 0x8,  //forward skip and playback
      Reverse   = 0x9,  //reverse skip and playback
      TrackSkip = 0xa,  //start track skipping
      TrackCue  = 0xb,  //start track cueing
      DoorClose = 0xc,  //close the door
      DoorOpen  = 0xd,  //open the door
    };};

    struct Request { enum : u32 {
      AbsoluteTime       = 0x0,
      RelativeTime       = 0x1,
      TrackInformation   = 0x2,
      DiscCompletionTime = 0x3,
      DiscTracks         = 0x4,  //start/end track numbers
      TrackStartTime     = 0x5,  //start time of specific track
      ErrorInformation   = 0x6,
      SubcodeError       = 0xe,
      NotReady           = 0xf,  //not ready to comply with the current command
    };};

    //cdd.cpp
    auto load(Node::Object) -> void;
    auto unload(Node::Object) -> void;

    auto clock() -> void;
    auto advance() -> void;
    auto sample() -> void;
    auto position(s32 sector) -> double;
    auto process() -> void;
    auto valid() -> bool;
    auto checksum() -> void;
    auto insert() -> void;
    auto eject() -> void;
    auto power(bool reset) -> void;

    //serialization.cpp
    auto serialize(serializer&) -> void;

    CD::Session session;
    IRQ irq;
    n16 counter;

    struct DAC {
      Node::Audio::Stream stream;

      //cdd-dac.cpp
      auto load(Node::Object) -> void;
      auto unload(Node::Object) -> void;

      auto sample(i16 left, i16 right) -> void;
      auto reconfigure() -> void;
      auto power(bool reset) -> void;

      n1  rate;        //0 = normal, 1 = double
      n2  deemphasis;  //0 = off, 1 = 44100hz, 2 = 32000hz, 3 = 48000hz
      n16 attenuator;  //only d6-d15 are used for the coefficient
      n16 attenuated;  //current coefficent
    } dac;

    struct IO {
      n4  status = Status::NoDisc;
      n4  seeking;  //status after seeking (Playing or Paused)
      n16 latency;
      i32 sector;   //current frame#
      n16 sample;   //current audio sample# within current frame
      n7  track;    //current track#
    } io;

    n1 hostClockEnable;
    n1 statusPending;
    n4 status [10];
    n4 command[10];
  } cdd;

  struct Timer {
    //timer.cpp
    auto clock() -> void;
    auto power(bool reset) -> void;

    //serialization.cpp
    auto serialize(serializer&) -> void;

    IRQ irq;
    n8 frequency;
    n8 counter;
  } timer;

  struct GPU {
    //gpu.cpp
    auto step(u32 clocks) -> void;
    auto read(n19 address) -> n4;
    auto write(n19 address, n4 data) -> void;
    auto render(n19 address, n9 width) -> void;
    auto start() -> void;
    auto power(bool reset) -> void;

    //serialization.cpp
    auto serialize(serializer&) -> void;

    IRQ irq;

    struct Font {
      struct Color {
        n4 background;
        n4 foreground;
      } color;
      n16 data;
    } font;

    struct Stamp {
      n1 repeat;
      struct Tile {
        n1 size;  //0 = 16x16, 1 = 32x32
      } tile;
      struct Map {
        n1  size;  //0 = 1x1, 1 = 16x16
        n18 base;
        n19 address;
      } map;
    } stamp;

    struct Image {
      n18 base;
      n6  offset;
      n5  vcells;
      n8  vdots;
      n9  hdots;
      n19 address;
    } image;

    struct Vector {
      n18 base;
      n17 address;
    } vector;

    n1  active;
    n32 counter;
    n32 period;
  } gpu;

  struct PCM {
    Node::Audio::Stream stream;
    Memory::Writable<n8> ram;

    //pcm.cpp
    auto load(Node::Object) -> void;
    auto unload(Node::Object) -> void;

    auto clock() -> void;
    auto read(n13 address, n8 data) -> n8;
    auto write(n13 address, n8 data) -> void;
    auto power(bool reset) -> void;

    //serialization.cpp
    auto serialize(serializer&) -> void;

    struct IO {
      n1 enable;
      n4 bank;
      n3 channel;
    } io;

    struct Channel {
      //serialization.cpp
      auto serialize(serializer&) -> void;

      n1  enable;
      n8  envelope;
      n8  pan = 0xff;
      n16 step;
      n16 loop;
      n8  start;
      n27 address;
    } channels[8];
  } pcm;
};

extern MCD mcd;
