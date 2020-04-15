//PC Engine CD-ROM

struct PCD : Thread {
  Node::Component node;
  Node::Port tray;
  Node::Peripheral disc;
  Shared::File fd;
  CD::Session session;
  Memory::Writable<uint8> wram;   //64KB

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
  auto read(uint4 address) -> uint8;
  auto write(uint4 address, uint8 data) -> void;

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
    auto main() -> void;
    auto readData() -> uint8;
    auto update() -> void;
    auto messageInput() -> void;
    auto messageOutput() -> void;
    auto dataInput() -> void;
    auto dataOutput() -> void;
    auto commandTestUnitReady() -> void;
    auto commandRead() -> void;
    auto commandGetDirectoryInformation() -> void;
    auto replyStatusOK() -> void;
    auto replyStatusError() -> void;

    //serialization.cpp
    auto serialize(serializer&) -> void;

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
    Memory::Writable<uint8> memory;  //64KB

    //adpcm.cpp
    auto load(Node::Object) -> void;
    auto unload() -> void;

    //serialization.cpp
    auto serialize(serializer&) -> void;
  } adpcm;

  struct IO {
    uint8 mdr[16];
  } io;
};

extern PCD pcd;
