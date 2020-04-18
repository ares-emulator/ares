//PC Engine CD-ROM

struct PCD : Thread {
  Node::Component node;
  Node::Port tray;
  Node::Peripheral disc;
  Shared::File fd;
  CD::Session session;
  Memory::Writable<uint8> wram;   //64KB

  struct Debugger {
    //debugger.cpp
    auto load(Node::Object) -> void;

    struct Memory {
      Node::Memory wram;
      Node::Memory adpcm;
      Node::Memory bram;
    } memory;
  } debugger;

  static auto Present() -> bool { return true; }

  auto manifest() const -> string { return information.manifest; }
  auto name() const -> string { return information.name; }

  //pcd.cpp
  auto load(Node::Object) -> void;
  auto unload() -> void;

  auto allocate(Node::Port) -> Node::Peripheral;
  auto connect() -> void;
  auto disconnect() -> void;

  auto save() -> void;
  auto main() -> void;
  auto step(uint clocks) -> void;
  auto irqLine() const -> bool;
  auto power() -> void;

  //io.cpp
  auto read(uint10 address) -> uint8;
  auto write(uint10 address, uint8 data) -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  struct Information {
    string manifest;
    string name;
  } information;

  struct IRQ {
    uint1 enable;
    uint1 pending;
  };

  struct BRAM {
    Memory::Writable<uint8> memory;  //2KB

    //bram.cpp
    auto load(Node::Object) -> void;
    auto unload() -> void;

    auto read(uint11 address) -> uint8;
    auto write(uint11 address, uint8 data) -> void;

    //serialization.cpp
    auto serialize(serializer&) -> void;

    uint1 writable;
  } bram;

  struct SCSI {
    //scsi.cpp
    auto clock() -> void;
    auto clockRead() -> void;
    auto readData() -> uint8;
    auto update() -> void;
    auto messageInput() -> void;
    auto messageOutput() -> void;
    auto dataInput() -> void;
    auto dataOutput() -> void;
    auto commandTestUnitReady() -> void;
    auto commandRead() -> void;
    auto commandGetDirectoryInformation() -> void;
    auto commandIgnore() -> void;
    auto replyStatusOK() -> void;
    auto replyStatusError() -> void;

    //serialization.cpp
    auto serialize(serializer&) -> void;

    uint32 counter;

    struct {
      IRQ ready;
      IRQ completed;
    } irq;

    uint1 reset;        //RST
    uint1 attention;    //ATN
    uint1 acknowledge;  //ACK
    uint1 select;       //SEL
    uint1 input;        //I/O (1 = input, 0 = output)
    uint1 control;      //C/D (1 = control, 0 = data)
    uint1 message;      //MSG
    uint1 request;      //REQ
    uint1 busy;         //BSY

    uint8 data;

    uint1 wasReset;
    uint1 selected;

    uint1 messageAfterStatus;
    uint1 messageSent;
    uint1 statusSent;
    uint1 dataTransferred;

    struct Buffer {
       uint8 data[4_KiB];
      uint12 size;
      uint12 index;
    };

    Buffer commandBuffer;
    Buffer dataBuffer;

    struct Read {
      int lba;
      int end;
    } read;
  } scsi;

  struct ADPCM {
    Node::Stream stream;
    MSM5205 msm5205;
    Memory::Writable<uint8> memory;  //64KB

    //adpcm.cpp
    auto load(Node::Object) -> void;
    auto unload() -> void;

    auto clock() -> void;
    auto control(uint8 shadow, uint8 data) -> void;
    auto play() -> void;
    auto stop(bool) -> void;
    auto power() -> void;

    auto readRAM() -> uint8;
    auto writeRAM(uint8 data) -> void;

    //serialization.cpp
    auto serialize(serializer&) -> void;

    uint32 counter;

    struct {
      IRQ halfPlay;
      IRQ fullPlay;
    } irq;

     uint1 idle;
     uint1 nibble;

     uint1 enable;
     uint1 run;

     uint1 stopped;
     uint1 playing;
     uint1 reading;
     uint1 writing;
     uint1 repeat;
     uint4 divider;  //0x01-0x10
     uint4 period;

    uint16 latch;
    uint16 length;
    uint16 readCounter;
    uint16 readAddress;
    uint16 writeCounter;
    uint16 writeAddress;
    uint16 startAddress;
    uint16 halfAddress;
    uint16 endAddress;
  } adpcm;

  struct IO {
    uint8 mdr[16];
  } io;
};

extern PCD pcd;
