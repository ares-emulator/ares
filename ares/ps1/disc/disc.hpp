struct Disc : Memory::Interface {
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

  enum ErrorCode : u8 {
    ErrorCode_InvalidParameterValue = 0x10,
    ErrorCode_InvalidParameterCount = 0x20,
    ErrorCode_InvalidCommand = 0x40,
    ErrorCode_CannotRespondYet = 0x80,
    ErrorCode_SeekFailed = 0x04,
    ErrorCode_DoorOpen = 0x08,
  };

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
  auto canReadDMA() -> bool;
  auto readDMA() -> u32;
  auto readByte(u32 address) -> u32;
  auto readHalf(u32 address) -> u32;
  auto readWord(u32 address) -> u32;
  auto writeByte(u32 address, u32 data) -> void;
  auto writeHalf(u32 address, u32 data) -> void;
  auto writeWord(u32 address, u32 data) -> void;

  enum class ResponseType : u8 {
    None,
    Ready,       //INT 1
    Complete,    //INT 2
    Acknowledge, //INT 3
    End,         //INT 4
    Error,       //INT 5
  };

  //command.cpp
  auto status() -> u8;
  auto mode() -> u8;
  auto ack() -> void;
  auto error(u8 code) -> void;
  auto queueResponse(ResponseType type, std::initializer_list<u8> response) -> void;
  auto flushDeferredResponse() -> void;

  auto executeCommand(u8 operation) -> void;
  auto commandTest() -> void;
  auto commandInvalid() -> void;
  auto commandNop() -> void;
  auto commandSetLoc() -> void;
  auto commandPlay() -> void;
  auto commandForward() -> void;
  auto commandBackward() -> void;
  auto commandReadN() -> void;
  auto commandStandby() -> void;
  auto commandStop() -> void;
  auto commandPause() -> void;
  auto commandInit() -> void;
  auto commandMute() -> void;
  auto commandDemute() -> void;
  auto commandSetFilter() -> void;
  auto commandSetMode() -> void;
  auto commandGetParam() -> void;
  auto commandGetlocL() -> void;
  auto commandGetlocP() -> void;
  auto commandSetSession() -> void;
  auto commandGetTN() -> void;
  auto commandGetTD() -> void;
  auto commandSeekL() -> void;
  auto commandSeekP() -> void;
  auto commandTestStartReadSCEX() -> void;
  auto commandTestStopReadSCEX() -> void;
  auto commandTestControllerDate() -> void;
  auto commandGetID() -> void;
  auto commandReadS() -> void;
  auto commandReadToc() -> void;
  auto commandUnimplemented(u8, maybe<u8> = nothing) -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  struct Drive;
  struct CDDA;
  struct CDXA;

  struct Drive {
    Disc& self;
    Drive(Disc& self) : self(self) {}

    enum class SeekType : u8 { None, SeekL, SeekP };
    enum class PendingOperation : u8 { None, Read, Play};

    maybe<CD::Session&> session;
    maybe<CDDA&> cdda;
    maybe<CDXA&> cdxa;

    //drive.cpp
    auto distance() const -> u32;
    auto updateSubQ() -> void;
    auto clockSector() -> void;

    struct LBA {
      s32 current;
      s32 request;
      n1  pending; //unprocessed setLoc
    } lba;

    struct Sector {
      u8  data[2448];
      u8  subq[10];
      u16 offset;
      u8 track;
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

    u32 recentlyReset;
    u32 seeking;
    u32 seekDelay;
    u16 seekRetries;
    SeekType seekType = SeekType::None;
    PendingOperation pendingOperation = PendingOperation::None;
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
    template<bool isStereo, bool is8bit> auto decodeADPCM(n1 halfSampleRate) -> void;
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

  struct Command {
    struct Current {
      u8  command;
      u8  invocation;
      n1  pending;
      s32 counter;
    } current;

    struct Pending {
      u8 command;
      n1 pending;
    } queued;

    struct Transfer {
      s32 counter;
      n1  started;
      u8  command;
    } transfer;

  } command;


  struct IRQ {
    //irq.cpp
    auto poll() -> void;
    auto pending() -> bool;

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

    struct DeferredData {
      ResponseType type;
      queue<u8[16]> data;
    };

    struct Deferred {
      DeferredData ready;       //INT1
      DeferredData complete;    //INT2
      DeferredData acknowledge; //INT3,4,5
    } deferred;
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
    n1 playingCDDA;
  } ssr;

  struct IO {
    n2 index;
    n1 soundMapEnable;
    n1 sectorBufferReadRequest;
    n1 sectorBufferWriteRequest;
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
