struct Disc : Thread {
  Node::Component node;
  Node::Port tray;
  Node::Peripheral cd;
  Shared::File fd;
  CD::Session session;

  auto manifest() const -> string { return information.manifest; }
  auto name() const -> string { return information.name; }
  auto region() const -> string { return information.region; }
  auto audio() const -> bool { return information.region == ""; }
  auto executable() const -> bool { return information.executable; }

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
  auto readDMA() -> u32;
  auto readByte(u32 address) -> u32;
  auto readHalf(u32 address) -> u32;
  auto readWord(u32 address) -> u32;
  auto writeByte(u32 address, u32 data) -> void;
  auto writeHalf(u32 address, u32 data) -> void;
  auto writeWord(u32 address, u32 data) -> void;

  //command.cpp
  auto status() -> u8;
  auto command(u8 operation) -> void;
  auto commandTest() -> void;
  auto commandGetStatus() -> void;
  auto commandSetLocation() -> void;
  auto commandPlay() -> void;
  auto commandFastForward() -> void;
  auto commandRewind() -> void;
  auto commandReadWithRetry() -> void;
  auto commandReadWithoutRetry() -> void;
  auto commandStop() -> void;
  auto commandPause() -> void;
  auto commandInitialize() -> void;
  auto commandMute() -> void;
  auto commandUnmute() -> void;
  auto commandSetMode() -> void;
  auto commandGetFirstAndLastTrackNumbers() -> void;
  auto commandGetTrackStart() -> void;
  auto commandSeekData() -> void;
  auto commandSeekCDDA() -> void;
  auto commandTestControllerDate() -> void;
  auto commandGetID() -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  struct Drive;
  struct CDDA;

  struct Drive {
    Disc& self;
    Drive(Disc& self) : self(self) {}

    maybe<CD::Session&> session;
    maybe<CDDA&> cdda;

    //drive.cpp
    auto distance() const -> uint;
    auto clockSector() -> void;

    struct LBA {
      int current = 0;
      int request = 0;
      int seeking = 0;
    } lba;

    struct Sector {
       u8 data[2448];
      u16 offset;
    } sector;

    struct Mode {
      uint1 cdda;
      uint1 autoPause;
      uint1 report;
      uint1 xaFilter;
      uint1 ignore;
      uint1 sectorSize;
      uint1 xaADPCM;
      uint1 speed;
    } mode;
  } drive{*this};

  struct CDDA {
    Disc& self;
    CDDA(Disc& self) : self(self) {}

    maybe<Drive&> drive;
    Node::Stream stream;

    //cdda.cpp
    auto load(Node::Object) -> void;
    auto unload() -> void;

    auto clockSector() -> void;
    auto clockSample() -> void;

    enum class PlayMode : uint {
      Normal,
      FastForward,
      Rewind,
    } playMode = PlayMode::Normal;

    uint8 volume[4];
    uint8 volumeLatch[4];

    struct Sample {
      i16 left;
      i16 right;
    } sample;
  } cdda{*this};

  struct Event {
     u8 command = 0;
    i32 counter = 0;
     u8 invocation = 0;
  } event;

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

  struct PrimaryStatusRegister {
  } psr;

  struct SecondaryStatusRegister {
    uint1 error;
    uint1 motorOn = 1;
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

  struct Counter {
    i32 sector = 0;
    i32 cdda = 0;
    i32 report = 0;
  } counter;

//unserialized:
  struct Information {
    string manifest;
    string name;
    string region;
    boolean executable;
  } information;
};

extern Disc disc;
