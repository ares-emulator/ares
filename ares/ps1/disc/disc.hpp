struct Disc : Thread, Memory::Interface {
  Node::Object node;
  Node::Port tray;
  Node::Peripheral cd;
  VFS::Pak pak;
  VFS::File fd;
  CD::Session session;

  struct Debugger {
    //debugger.cpp
    auto load(Node::Object) -> void;
    auto commandPrologue(u8 operation, maybe<u8> suboperation = nothing) -> void;
    auto commandEpilogue(u8 operation, maybe<u8> suboperation = nothing) -> void;
    auto read(s32 lba) -> void;

    struct Tracer {
      Node::Debugger::Tracer::Notification command;
      Node::Debugger::Tracer::Notification read;
    } tracer;

  private:
    string _command;
  } debugger;

  auto title() const -> string { return information.title; }
  auto region() const -> string { return information.region; }
  auto audioCD() const -> bool { return information.audio; }
  auto executable() const -> bool { return information.executable; }
  auto noDisc() const -> bool { return !title(); }

  //disc.cpp
  auto load(Node::Object) -> void;
  auto unload() -> void;

  auto allocate(Node::Port) -> Node::Peripheral;
  auto connect() -> void;
  auto disconnect() -> void;

  auto main() -> void;
  auto step(u32 clocks) -> void;
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
  auto commandGetLocationReading() -> void;
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
    auto distance() const -> u32;
    auto clockSector() -> void;

    struct LBA {
      s32 current;
      s32 request;
      s32 seeking;
    } lba;

    struct Sector {
      u8  data[2448];
      u16 offset;
    } sector;

    struct Mode {
      n1 cdda;
      n1 autoPause;
      n1 report;
      n1 xaFilter;
      n1 ignore;
      n1 sectorSize;
      n1 xaADPCM;
      n1 speed;
    } mode;

    n32 seeking;
  } drive{*this};

  struct Audio {
    n1 mute;
    n1 muteADPCM;
    n8 volume[4];
    n8 volumeLatch[4];
  } audio;

  struct CDDA {
    Disc& self;
    CDDA(Disc& self) : self(self) {}

    maybe<Drive&> drive;
    Node::Audio::Stream stream;

    //cdda.cpp
    auto load(Node::Object) -> void;
    auto unload(Node::Object) -> void;

    auto clockSector() -> void;
    auto clockSample() -> void;

    enum class PlayMode : u32 {
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
    auto unload(Node::Object) -> void;

    auto clockSector() -> void;
    auto clockSample() -> void;
    template<bool isStereo, bool is8bit> auto decodeADPCM() -> void;
    template<bool isStereo, bool is8bit> auto decodeBlock(s16* output, u16 address) -> void;

    struct Filter {
      n8 file;
      n8 channel;
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
      n1 enable;
      n1 flag;
    };

    n5 flag;
    n5 mask;

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
    n1 error;
    n1 motorOn = 1;
    n1 seekError;
    n1 idError;
    n1 shellOpen;
    n1 reading;
    n1 seeking;
    n1 playingCDDA;
  } ssr;

  struct IO {
    n2 index;
  } io;

  struct Counter {
    s32 sector;
    s32 cdda;
    s32 cdxa;
    s32 report;
  } counter;

//unserialized:
  struct Information {
    string title;
    string region;
    boolean audio;
    boolean executable;
  } information;
};

extern Disc disc;
