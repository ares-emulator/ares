struct Disc : Memory::Interface {
  Node::Object node;
  Node::Port tray;
  Node::Setting::String lidSetting;
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
  auto audioCD() const -> bool {
    if(session.firstTrack < 100 && session.tracks[session.firstTrack]) {
      return session.tracks[session.firstTrack].isAudio();
    }
    return information.audio;
  }
  auto executable() const -> bool { return information.executable; }
  auto noDisc() const -> bool { return !fd; }
  auto canReadMedia() const -> bool { return fd && !lidOpen && !drive.shellOpening; }
  bool lidOpen = false;  // Physical input, independent of the sticky shell-open status bit.
  bool manualLidControl = false;
  auto setLidOpen(bool open, bool manual = true) -> void;
  auto synchronizeLidSetting() -> void;
  auto invalidateMedia(bool forSwap = false) -> void;
  auto discRegionSuffix() const -> u8;
  auto discRegionMatches() const -> bool;

  struct ControllerProfile { const char* name; u8 date[4]; };
  static constexpr ControllerProfile controllerProfiles[] = {
    {"C0a", {0x94, 0x09, 0x19, 0xc0}}, {"C0b", {0x94, 0x11, 0x18, 0xc0}},
    {"C1a", {0x95, 0x05, 0x16, 0xc1}}, {"C1b", {0x95, 0x07, 0x24, 0xc1}},
    {"D1", {0x95, 0x07, 0x24, 0xd1}}, {"C2-VCD", {0x96, 0x08, 0x15, 0xc2}},
    {"C1-Yaroze", {0x96, 0x08, 0x18, 0xc1}}, {"C2a-J", {0x96, 0x09, 0x12, 0xc2}},
    {"C2a", {0x97, 0x01, 0x10, 0xc2}}, {"C2b", {0x97, 0x08, 0x14, 0xc2}},
    {"C3a", {0x98, 0x06, 0x10, 0xc3}}, {"C3b", {0x99, 0x02, 0x01, 0xc3}},
    {"C3c", {0xa1, 0x03, 0x06, 0xc3}},
  };
  u8 controllerVersion = 2;  // C1a; version identity and known command availability, not complete firmware emulation.
  auto setControllerVersion(string name) -> bool;

  //disc.cpp
  auto load(Node::Object) -> void;
  auto unload() -> void;

  auto allocate(Node::Port) -> Node::Peripheral;
  auto connect() -> void;
  auto disconnect(bool forSwap = true) -> void;

  auto main() -> void;
  auto step(u32 clocks) -> void;
  auto power(bool reset) -> void;

  //io.cpp
  auto canReadDMA() -> bool;
  auto readDMA() -> u32;
  auto readData() -> u8;
  auto receiveSector(const u8* sector) -> void;
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
  auto queueResponse(ResponseType type, std::initializer_list<u8> response, bool asynchronous = false) -> void;
  auto flushDeferredResponse() -> void;
  auto publishResponse(ResponseType type) -> void;
  auto scheduleAsyncResponse(u32 delay = 0, bool force = false) -> void;
  auto updateCommandEvent() -> void;
  auto beginCommand(u8 operation) -> void;
  auto scheduleSecondResponse(u8 operation, u32 clocks) -> void;
  auto completeStatusResponse() -> void;
  auto stopDriveActivity() -> void;
  auto startMotor() -> void;
  auto resetAudioDecoder() -> void;
  auto clearSectorBuffers() -> void;
  auto clearStreamBuffers() -> void;
  auto softReset() -> u32;

  struct ParameterCount { u8 minimum, maximum; };
  static auto parameterCount(u8 operation) -> ParameterCount;

  auto executeCommand(u8 operation, bool secondResponse = false) -> void;
  auto commandTest() -> void;
  auto commandInvalid() -> void;
  auto commandNop() -> void;
  auto commandSetLoc() -> void;
  auto commandPlay() -> void;
  auto commandForward() -> void;
  auto commandBackward() -> void;
  auto commandReadN() -> void;
  auto commandMotorOn(bool secondResponse) -> void;
  auto commandStop(bool secondResponse) -> void;
  auto commandPause(bool secondResponse) -> void;
  auto commandInit(bool secondResponse) -> void;
  auto commandMute() -> void;
  auto commandDemute() -> void;
  auto commandSetFilter() -> void;
  auto commandSetMode() -> void;
  auto commandGetParam() -> void;
  auto commandGetlocL() -> void;
  auto commandGetlocP() -> void;
  auto commandSetSession(bool secondResponse) -> void;
  auto commandGetTN() -> void;
  auto commandGetTD() -> void;
  auto commandSeekL() -> void;
  auto commandSeekP() -> void;
  auto commandTestStartReadSCEX() -> void;
  auto commandTestStopReadSCEX() -> void;
  auto commandTestControllerDate() -> void;
  auto commandGetID(bool secondResponse) -> void;
  auto commandReadS() -> void;
  auto commandReadToc(bool secondResponse) -> void;

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
    auto distance(s32 target) -> u32;
    auto seekJitter() -> u32;
    auto beginSeek() -> void;
    auto finishSeek() -> void;
    auto changeSpeed(bool doubleSpeed) -> void;
    auto beginStream(PendingOperation operation, bool afterSeek = false) -> void;
    auto updateSubQ() -> bool;
    auto sampleSubQ(s32 position) -> bool;
    auto synthesizeLeadOutQ(s32 position) -> bool;
    auto updatePosition(bool logical = false) -> void;
    auto updateSeekPosition() -> void;
    auto setHold(s32 position, s32 subq) -> void;
    auto cacheSubQ(const u8* q) -> bool;
    auto readSector(s32 position, u8* data) -> bool;
    auto clockSector() -> void;
    auto cacheHeader(const u8* sector) -> void;
    auto sectorsPerTrack(s32 position) const -> u32;
    auto pauseDelay() const -> u32;
    auto finishReading(bool stopMotor) -> void;

    struct LBA {
      s32 current;  // Next stream sector; equal to hold before a stream starts.
      s32 hold;     // Last consumed sector or logical seek/paused target.
      s32 request;
      s32 seek;
      s32 start;
      n1  pending; //unprocessed setLoc
    } lba;

    struct Physical {
      s32 position = 0;
      u64 age = 0;
      u32 carry = 0;
      bool pending = false;
    } physical;

    struct Sector {
      u8  data[2448];
      u8  subq[10];  // Last accepted mech Q; streaming samples two sectors ahead of decoded data.
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

    u8 header[8] = {};
    bool headerValid = false;
    u32 spinUp = 0;
    u32 shellOpening = 0;
    u32 resetDelay = 0;
    u32 seeking;
    u32 seekInterval = 0;
    bool seekVisible = false;
    bool streamStarting = false;
    bool activeStatusCleared = false;
    u8 startingStatus = 0;
    u32 speedChange = 0;
    u64 seekRandom[2] = {};
    SeekType seekType = SeekType::None;
    PendingOperation pendingOperation = PendingOperation::None;
    struct SessionChange {
      u8 number = 0;
      u32 remaining = 0;
      bool pending = false;
    } sessionChange;
  } drive{*this};

  struct Audio {
    // Bits0..15=L,16..31=R,32=XA source. One entry is always a complete stereo frame.
    queue<u64[44100 * 2]> frames;
    struct Sample { s16 left = 0, right = 0; } sample;
    auto pushFrame(s16 left, s16 right, bool xa = false) -> void;
    auto clockSample() -> void;
    n1 mute;
    n1 muteADPCM;
    n8 volume[4];
    n8 volumeLatch[4];
    auto mix(s16 left, s16 right, u32 channel) const -> s16 {
      auto first = s32(left) * u8(volume[channel]) >> 7;
      auto second = s32(right) * u8(volume[channel + 2]) >> 7;
      return sclamp<16>(first + second);
    }
  } audio;

  struct CDDA {
    Disc& self;
    CDDA(Disc& self) : self(self) {}

    maybe<Drive&> drive;
    Node::Audio::Stream stream;

    //cdda.cpp
    auto load(Node::Object) -> void;
    auto unload(Node::Object) -> void;

    auto clockSector(bool validSubQ) -> void;
    auto report(bool validSubQ) -> void;
    auto beginReport() -> void;
    auto peak(u8 channel) const -> u16;
    u8 reportDelay = 0;
    u8 reportFrame = 0xff;
    s8 scanStep = 0;
    bool started = false;
    u8 playTrack = 0;
    s32 holdLBA = 0;
    bool autoPausePending = false;

    enum class PlayMode : u32 {
      Normal,
      FastForward,
      Rewind,
    } playMode;

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
    auto decodeSector(s16* output) -> u32;
    auto resample(const s16* input, u32 frames, bool stereo, bool halfRate) -> void;
    auto pushFrame(s16 left, s16 right) -> void;
    template<bool isStereo, bool is8bit> auto decodeADPCM(s16* output) -> void;
    template<bool isStereo, bool is8bit> auto decodeBlock(s16* output, u16 address) -> void;

    struct Filter {
      n8 file;
      n8 channel;
    } filter;

    struct CurrentFile {
      u8 file = 0;
      u8 channel = 0;
      bool selected = false;
    } current;

    s32 previousSamples[4];
    s16 resampleRing[2][32] = {};
    n5 resamplePosition = 0;
    u8 resampleStep = 6;
  } cdxa{*this};

  struct Command {
    struct Event {
      u8  command;
      n1  pending;
      s32 counter;
    } first, second;
    u32 elapsed;
    bool active;
  } command;


  struct IRQ {
    //irq.cpp
    auto poll() -> void;
    auto pending() -> bool;

    n5 flag;
    n5 mask;
    u32 acknowledgeAge = 1000;
  } irq;

  struct FIFO {
    queue<u8[16]> parameter;
    queue<u8[16]> response;
    u8 responseLatch[16] = {};
    n4 responsePosition;

    struct DataBuffer {
      u8 bytes[2340] = {};
      u16 length = 0;
      u16 position = 0;
      u8 padding = 0;
      bool exhausted = false;

      auto empty() const -> bool { return position >= length; }
      auto size() const -> u32 { return length - position; }
      auto flush() -> void { length = position = padding = 0; exhausted = false; }
      auto write(u8 value) -> void { if(length < sizeof(bytes)) bytes[length++] = value; }
      auto read() -> u8 {
        if(empty()) return padding;
        auto value = bytes[position++];
        if(empty()) {
          // Documented decoder overread padding (DISC-009).
          u32 distance = length == 2048 ? 8 : 4;
          padding = bytes[length >= distance ? length - distance : 0];
          exhausted = true;
        }
        return value;
      }
    } data;

    // Reference-derived decoder ring. BFRD snapshots the selected slot into the locked host transfer.
    static constexpr u32 SectorBuffers = 8;
    DataBuffer sectors[SectorBuffers];
    n3 sectorRead;
    n3 sectorWrite;
    bool sectorPending = false;
    bool hostLoaded = false;
    u8 sectorFile = 0;
    u8 sectorChannel = 0;
    bool sectorNeedsFilter = false;

    struct DeferredData {
      ResponseType type;
      queue<u8[16]> data;
    };

    struct Deferred : DeferredData {
      s32 counter;
      bool scheduled;
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
    s32 audio;
  } counter;
  u32 clockAccumulator = 0;

//unserialized:
  struct Information {
    string title;
    string region;
    boolean audio;
    boolean executable;
  } information;
};

extern Disc disc;
