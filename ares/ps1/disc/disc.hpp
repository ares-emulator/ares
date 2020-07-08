struct Disc : Thread {
  Node::Component node;
  Node::Port tray;
  Node::Peripheral cd;
  Shared::File fd;
  CD::Session session;

  auto manifest() const -> string { return information.manifest; }
  auto name() const -> string { return information.name; }

  //disc.cpp
  auto load(Node::Object) -> void;
  auto unload() -> void;

  auto allocate(Node::Port) -> Node::Peripheral;
  auto connect() -> void;
  auto disconnect() -> void;

  auto main() -> void;
  auto step(uint clocks) -> void;
  auto power(bool reset) -> void;

  //io.cpp
  auto readByte(u32 address) -> u32;
  auto readHalf(u32 address) -> u32;
  auto readWord(u32 address) -> u32;
  auto writeByte(u32 address, u32 data) -> void;
  auto writeHalf(u32 address, u32 data) -> void;
  auto writeWord(u32 address, u32 data) -> void;

  //command.cpp
  auto command(u8 operation) -> void;
  auto commandGetStatus() -> void;
  auto commandGetFirstAndLastTrackNumbers() -> void;
  auto commandGetTrackStart() -> void;
  auto commandTest() -> void;
  auto commandTestControllerDate() -> void;
  auto commandGetID() -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  struct Information {
    string manifest;
    string name;
  } information;

  struct IRQ {
    //irq.cpp
    auto poll() -> void;

    struct Source {
       uint1 enable;
       uint1 flag;
      uint32 delay;
    };

    uint5 flag;
    uint5 mask;

    Source ready;        //INT1
    Source complete;     //INT2
    Source acknowledge;  //INT3
    Source end;          //INT4
    Source error;        //INT5
    Source unknown;      //INT8
    Source start;        //INT10
  } irq;

  struct FIFO {
    queue<u8> parameter;
    queue<u8> response;
    queue<u8> data;
  } fifo;

  struct ADPCM {
    uint1 mute;
  } adpcm;

  struct CDDA {
    uint8 volume[4];
    uint8 volumeLatch[4];
  } cdda;

  struct PrimaryStatusRegister {
  } psr;

  struct SecondaryStatusRegister {
    uint1 error;
    uint1 motorOn;
    uint1 seekError;
    uint1 idError;
    uint1 shellOpen;
    uint1 reading;
    uint1 seeking;
    uint1 playingCDDA;
  } ssr;

  struct IO {
    uint2 index;
  } io;
};

extern Disc disc;
