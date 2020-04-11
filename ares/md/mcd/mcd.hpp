//Mega CD

struct MCD : M68K, Thread {
  Node::Component node;
  Node::Port tray;
  Node::Peripheral disc;
  Shared::File fd;
  Memory::Readable<uint16> bios;  //BIOS ROM
  Memory::Writable<uint16> pram;  //program RAM
  Memory::Writable<uint16> wram;  //work RAM
  Memory::Writable< uint8> bram;  //backup RAM

  struct Debugger {
    //debugger.cpp
    auto load(Node::Object) -> void;
    auto instruction() -> void;
    auto interrupt(string_view) -> void;

    struct Memory {
      Node::Memory pram;
      Node::Memory wram;
      Node::Memory bram;
    } memory;

    struct Tracer {
      Node::Instruction instruction;
      Node::Notification interrupt;
    } tracer;
  } debugger;

  auto manifest() const -> string { return information.manifest; }
  auto name() const -> string { return information.name; }

  //mcd.cpp
  auto load(Node::Object) -> void;
  auto unload() -> void;

  auto allocate(Node::Port) -> Node::Peripheral;
  auto connect() -> void;
  auto disconnect() -> void;

  auto main() -> void;
  auto step(uint clocks) -> void;
  auto idle(uint clocks) -> void override;
  auto wait(uint clocks) -> void override;
  auto power(bool reset) -> void;

  //bus.cpp
  auto read(uint1 upper, uint1 lower, uint24 address, uint16 data = 0) -> uint16 override;
  auto write(uint1 upper, uint1 lower, uint24 address, uint16 data) -> void override;

  //bus-external.cpp
  auto external_read(uint1 upper, uint1 lower, uint22 address, uint16 data) -> uint16;
  auto external_write(uint1 upper, uint1 lower, uint22 address, uint16 data) -> void;

  //io.cpp
  auto readIO(uint1 upper, uint1 lower, uint24 address, uint16 data) -> uint16;
  auto writeIO(uint1 upper, uint1 lower, uint24 address, uint16 data) -> void;

  //io-external.cpp
  auto external_readIO(uint1 upper, uint1 lower, uint24 address, uint16 data) -> uint16;
  auto external_writeIO(uint1 upper, uint1 lower, uint24 address, uint16 data) -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  struct Information {
    string manifest;
    string name;
  } information;

  struct Counter {
    uint16 divider;
    uint16 dma;
    double pcm = 0.0;
  } counter;

  struct IO {
     uint1 run;
     uint1 request;
     uint1 halt = 1;

    uint16 wramLatch;
     uint1 wramMode;  //MODE: 0 = 2mbit mode, 1 = 1mbit mode
     uint1 wramSwitch;
     uint1 wramSelect;
     uint2 wramPriority;
     uint2 pramBank;
     uint8 pramProtect;
  } io;

  struct LED {
    uint1 red;
    uint1 green;
  } led;

  struct IRQ {
    //irq.cpp
    auto raise() -> bool;
    auto lower() -> bool;
    static auto synchronize() -> void;

    //serialization.cpp
    auto serialize(serializer&) -> void;

    uint1 enable;
    uint1 pending;
  };

  struct IRQs {
    uint1 pending;

    IRQ reset;
    IRQ subcode;
  } irq;

  struct External {
    IRQ irq;
  } external;

  struct Communcation {
     uint8 cfm;
     uint8 cfs;
    uint16 command[8];
    uint16 status [8];
  } communication;

  struct CDC {
    //cdc.cpp
    auto poll() -> void;
    auto clock() -> void;
    auto decode(int sector) -> void;
    auto read() -> uint8;
    auto write(uint8 data) -> void;
    auto power(bool reset) -> void;

    //serialization.cpp
    auto serialize(serializer&) -> void;

    Memory::Writable<uint16> ram;

     uint4 address;
    uint12 stopwatch;

    struct IRQ : MCD::IRQ {
      MCD::IRQ decoder;   //DECEIN + DECI
      MCD::IRQ transfer;  //DTEIEN + DTEI
      MCD::IRQ command;   //CMDIEN + CMDI
    } irq;

    struct Command {
      uint8 fifo[8];  //COMIN
      uint3 read;
      uint3 write;
      uint1 empty = 1;
    } command;

    struct Status {
      uint8 fifo[8];    //SBOUT
      uint3 read;
      uint3 write;
      uint1 empty = 1;
      uint1 enable;     //SOUTEN
      uint1 active;     //STEN
      uint1 busy;       //STBSY
      uint1 wait;       //STWAI
    } status;

    struct Transfer {
      //cdc-transfer.cpp
      auto dma() -> void;
      auto read() -> uint16;
      auto start() -> void;
      auto complete() -> void;
      auto stop() -> void;

      //serialization.cpp
      auto serialize(serializer&) -> void;

       uint3 destination;
      uint19 address;

      uint16 source;
      uint16 target;
      uint16 pointer;
      uint12 length;

       uint1 enable;     //DOUTEN
       uint1 active;     //DTEN
       uint1 busy;       //DTBSY
       uint1 wait;       //DTWAI
       uint1 ready;      //DSR
       uint1 completed;  //EDT
    } transfer;

    enum : uint { Mode1 = 0, Mode2 = 1 };
    enum : uint { Form1 = 0, Form2 = 1 };
    struct Decoder {
      uint1 enable;  //DECEN
      uint1 mode;    //MODE
      uint1 form;    //FORM
      uint1 valid;   //!VALST
    } decoder;

    struct Header {
      uint8 minute;
      uint8 second;
      uint8 frame;
      uint8 mode;
    } header;

    struct Subheader {
      uint8 file;
      uint8 channel;
      uint8 submode;
      uint8 coding;
    } subheader;

    struct Control {
      uint1 head;               //SHDREN: 0 = read header, 1 = read subheader
      uint1 mode;               //MODE
      uint1 form;               //FORM
      uint1 commandBreak;       //CMDBK
      uint1 modeByteCheck;      //MBCKRQ
      uint1 erasureRequest;     //ERAMRQ
      uint1 writeRequest;       //WRRQ
      uint1 pCodeCorrection;    //PRQ
      uint1 qCodeCorrection;    //QRQ
      uint1 autoCorrection;     //AUTOQ
      uint1 errorCorrection;    //E01RQ
      uint1 edcCorrection;      //EDCRQ
      uint1 correctionWrite;    //COWREN
      uint1 descramble;         //DSCREN
      uint1 syncDetection;      //SYDEN
      uint1 syncInterrupt;      //SYIEN
      uint1 erasureCorrection;  //ERAMSL
      uint1 statusTrigger;      //STENTRG
      uint1 statusControl;      //STENCTL
    } control;
  } cdc;

  struct CDD {
    struct Status { enum : uint {
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

    struct Command { enum : uint {
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

    struct Request { enum : uint {
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
    auto unload() -> void;

    auto clock() -> void;
    auto advance() -> void;
    auto sample() -> void;
    auto position(int sector) -> double;
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
    uint16 counter;

    struct DAC {
      Node::Stream stream;

      //cdd-dac.cpp
      auto load(Node::Object) -> void;
      auto unload() -> void;

      auto sample(int16 left, int16 right) -> void;
      auto reconfigure() -> void;
      auto power(bool reset) -> void;

       uint1 rate;        //0 = normal, 1 = double
       uint2 deemphasis;  //0 = off, 1 = 44100hz, 2 = 32000hz, 3 = 48000hz
      uint16 attenuator;  //only d6-d15 are used for the coefficient
      uint16 attenuated;  //current coefficent
    } dac;

    struct IO {
       uint4 status = Status::NoDisc;
       uint4 seeking;  //status after seeking (Playing or Paused)
      uint16 latency;
       int32 sector;   //current frame#
      uint16 sample;   //current audio sample# within current frame
       uint7 track;    //current track#
    } io;

    uint1 hostClockEnable;
    uint1 statusPending;
    uint4 status [10];
    uint4 command[10];
  } cdd;

  struct Timer {
    //timer.cpp
    auto clock() -> void;
    auto power(bool reset) -> void;

    //serialization.cpp
    auto serialize(serializer&) -> void;

    IRQ irq;
    uint8 counter;
  } timer;

  struct GPU {
    //gpu.cpp
    auto step(uint clocks) -> void;
    auto read(uint19 address) -> uint4;
    auto write(uint19 address, uint4 data) -> void;
    auto render(uint19 address, uint9 width) -> void;
    auto start() -> void;
    auto power(bool reset) -> void;

    //serialization.cpp
    auto serialize(serializer&) -> void;

    IRQ irq;

    struct Font {
      struct Color {
        uint4 background;
        uint4 foreground;
      } color;
      uint16 data;
    } font;

    struct Stamp {
      uint1 repeat;
      struct Tile {
        uint1 size;  //0 = 16x16, 1 = 32x32
      } tile;
      struct Map {
         uint1 size;  //0 = 1x1, 1 = 16x16
        uint18 base;
        uint19 address;
      } map;
    } stamp;

    struct Image {
      uint18 base;
       uint6 offset;
       uint5 vcells;
       uint8 vdots;
       uint9 hdots;
      uint19 address;
    } image;

    struct Vector {
      uint18 base;
      uint17 address;
    } vector;

     uint1 active;
    uint32 counter;
    uint32 period;
  } gpu;

  struct PCM {
    Node::Stream stream;
    Memory::Writable<uint8> ram;

    //pcm.cpp
    auto load(Node::Object) -> void;
    auto unload() -> void;

    auto clock() -> void;
    auto read(uint13 address, uint8 data) -> uint8;
    auto write(uint13 address, uint8 data) -> void;
    auto power(bool reset) -> void;

    //serialization.cpp
    auto serialize(serializer&) -> void;

    struct IO {
      uint1 enable;
      uint4 bank;
      uint3 channel;
    } io;

    struct Channel {
      //serialization.cpp
      auto serialize(serializer&) -> void;

       uint1 enable;
       uint8 envelope;
       uint8 pan = 0xff;
      uint16 step;
      uint16 loop;
       uint8 start;
      uint27 address;
    } channels[8];
  } pcm;
};

extern MCD mcd;
