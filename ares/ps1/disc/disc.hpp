struct Disc : Thread, Memory::Interface {
  Node::Object node;
  Node::Port tray;
  Node::Peripheral cd;
  Shared::File fd;
  CD::Session session;

  struct Debugger {
    //debugger.cpp
    auto load(Node::Object) -> void;
    auto commandPrologue(u8 operation, maybe<u8> suboperation = nothing) -> void;
    auto commandEpilogue(u8 operation, maybe<u8> suboperation = nothing) -> void;
    auto read(int lba) -> void;

    struct Tracer {
      Node::Debugger::Tracer::Notification command;
      Node::Debugger::Tracer::Notification read;
    } tracer;

  private:
    string _command;
  } debugger;

  auto manifest() const -> string { return information.manifest; }
  auto name() const -> string { return information.name; }
  auto region() const -> string { return information.region; }
  auto audioCD() const -> bool { return information.audio; }
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
  auto commandInvalid() -> void;
  auto commandGetStatus() -> void;
  auto commandSetLocation() -> void;
  auto commandPlay() -> void;
  auto commandFastForward() -> void;
  auto commandRewind() -> void;
  auto commandReadWithRetry() -> void;
  auto commandStop() -> void;
  auto commandPause() -> void;
  auto commandInitialize() -> void;
  auto commandMute() -> void;
  auto commandUnmute() -> void;
  auto commandSetFilter() -> void;
  auto commandSetMode() -> void;
  auto commandGetLocationPlaying() -> void;
  auto commandSetSession() -> void;
  auto commandGetFirstAndLastTrackNumbers() -> void;
  auto commandGetTrackStart() -> void;
  auto commandSeekData() -> void;
  auto commandSeekCDDA() -> void;
  auto commandTestControllerDate() -> void;
  auto commandGetID() -> void;
  auto commandReadWithoutRetry() -> void;
  auto commandUnimplemented(u8, maybe<u8> = nothing) -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  struct Drive;
  struct CDDA;
  struct CDXA;

  struct Drive {
    Disc& self;
    Drive(Disc& self) : self(self) {}

    maybe<CD::Session&> session;
    maybe<CDDA&> cdda;
    maybe<CDXA&> cdxa;

    //drive.cpp
    auto distance() const -> uint;
    auto clockSector() -> void;

    struct LBA {
      int current;
      int request;
      int seeking;
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

    uint32 seeking;
  } drive{*this};

  struct Audio {
    uint1 mute;
    uint1 muteADPCM;
    uint8 volume[4];
    uint8 volumeLatch[4];
  } audio;

  struct CDDA {
    Disc& self;
    CDDA(Disc& self) : self(self) {}

    maybe<Drive&> drive;
    Node::Audio::Stream stream;

    //cdda.cpp
    auto load(Node::Object) -> void;
    auto unload() -> void;

    auto clockSector() -> void;
    auto clockSample() -> void;

    enum class PlayMode : uint {
      Normal,
      FastForward,
      Rewind,
    } playMode;

    struct Sample {
      s16 left;
      s16 right;
    } sample;
  } cdda{*this};

  struct CDXA {
    Disc& self;
    CDXA(Disc& self) : self(self) {}

    maybe<Drive&> drive;
    Node::Audio::Stream stream;

    //cdxa.cpp
    auto load(Node::Object) -> void;
    auto unload() -> void;

    auto clockSector() -> void;
    auto clockSample() -> void;
    template<bool isStereo, bool is8bit> auto decodeADPCM() -> void;
    template<bool isStereo, bool is8bit> auto decodeBlock(s16* output, u16 address) -> void;

    struct Filter {
      uint8 file;
      uint8 channel;
    } filter;

    struct Sample {
      s16 left;
      s16 right;
    } sample;

    bool monaural;
    queue<s16[4032 * 8]> samples;
    s32 previousSamples[4];
  } cdxa{*this};

  struct Event {
     u8 command;
    s32 counter;
     u8 invocation;
     u8 queued;
  } event;

  struct IRQ {
    //irq.cpp
    auto poll() -> void;

    struct Source {
      uint1 enable;
      uint1 flag;
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
    queue<u8[16]> parameter;
    queue<u8[16]> response;
    queue<u8[2340]> data;
  } fifo;

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

  struct Counter {
    s32 sector;
    s32 cdda;
    s32 cdxa;
    s32 report;
  } counter;

//unserialized:
  struct Information {
    string manifest;
    string name;
    string region;
    boolean audio;
    boolean executable;
  } information;
};

extern Disc disc;
