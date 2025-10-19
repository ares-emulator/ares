//PC Engine CD-ROM

struct PCD : Thread {
  Node::Object node;
  Node::Port tray;
  Node::Peripheral disc;
  VFS::Pak pak;
  VFS::File fd;
  CD::Session session;
  Memory::Writable<n8> wram;  // 64KB
  Memory::Writable<n8> bram;  //  2KB
  //PC Engine Duo & LaserActive only:
  Memory::Readable<n8> bios;  //256KB
  Memory::Writable<n8> sram;  //192KB

  struct Debugger {
    //debugger.cpp
    auto load(Node::Object) -> void;

    struct Memory {
      Node::Debugger::Memory wram;
      Node::Debugger::Memory bram;
      Node::Debugger::Memory sram;
      Node::Debugger::Memory adpcm;
    } memory;
  } debugger;

  static auto Present() -> bool { return true; }

  auto title() const -> string { return information.title; }
  auto bramEnable() const -> bool { return io.bramEnable; }

  //pcd.cpp
  auto load(Node::Object) -> void;
  auto unload() -> void;

  auto allocate(Node::Port) -> Node::Peripheral;
  auto connect() -> void;
  auto disconnect() -> void;

  auto save() -> void;
  auto main() -> void;
  auto step(u32 clocks) -> void;
  auto irqLine() const -> bool;
  auto power() -> void;

  //io.cpp
  auto read(n8 bank, n13 address, n8 data) -> n8;
  auto write(n8 bank, n13 address, n8 data) -> void;

  auto readIO(n13 address, n8 data) -> n8;
  auto writeIO(n13 address, n8 data) -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

//private:
  struct Interrupt {
    auto poll() const -> bool { return line & enable; }
    auto raise() -> void { line = 1; }
    auto lower() -> void { line = 0; }

    n1 enable;
    n1 line;
  };

  struct Buffer {
    auto reset() -> void { reads = 0; writes = 0; }
    auto end() const -> bool { return reads >= writes; }
    auto read() -> n8 { return data[reads++]; }
    auto write(n8 value) -> void { data[writes++] = value; }

    n8  data[4_KiB];
    n12 reads;
    n12 writes;
  };

  struct Drive;
  struct SCSI;
  struct CDDA;
  struct ADPCM;
  struct Fader;
  struct LD;

  struct Drive {
    maybe<CD::Session&> session;

    enum class Mode : u32 {
      Inactive,  //drive is not running
      Seeking,   //seeking
      Reading,   //reading CD data
      Playing,   //playing CDDA audio
      Paused,    //paused CDDA audio
      Stopped,   //stopped CDDA audio
    };

    auto inactive() const -> bool { return mode == Mode::Inactive; }
    auto seeking()  const -> bool { return mode == Mode::Seeking;  }
    auto reading()  const -> bool { return mode == Mode::Reading;  }
    auto playing()  const -> bool { return mode == Mode::Playing;  }
    auto paused()   const -> bool { return mode == Mode::Paused;   }
    auto stopped()  const -> bool { return mode == Mode::Stopped;  }
    auto playingAudioTrack()  const -> bool { return playing() && session && session->tracks[track].isAudio(); }

    auto setInactive() -> void { mode = Mode::Inactive; }
    auto setReading()  -> void { mode = Mode::Reading;  }
    auto setPlaying()  -> void { mode = Mode::Playing;  }
    auto setPaused()   -> void { mode = Mode::Paused;   }
    auto setStopped()  -> void { mode = Mode::Stopped;  }

    auto inData() const -> bool {
      return reading() || (seeking() && seek == Mode::Reading);
    }

    auto inCDDA() const -> bool {
      return playing() || (seeking() && seek == Mode::Playing)
          || paused()  || (seeking() && seek == Mode::Paused)
          || stopped();
    }

    //drive.cpp
    auto distance() -> u32;
    auto position(s32 sector) -> double;
    auto seekRead() -> void;
    auto seekPlay() -> void;
    auto seekPause() -> void;
    auto read() -> bool;
    auto power() -> void;
    auto stop() -> void;
    auto play() -> void;
    auto pause() -> void;
    auto seekToTime(u8 hour, u8 minute, u8 second, u8 frame, bool startPaused) -> void;
    auto seekToRelativeTime(n7 track, u8 minute, u8 second, u8 frame, bool startPaused) -> void;
    auto seekToSector(s32 lba, bool startPaused) -> void;
    auto seekToTrack(n7 track, bool startPaused) -> void;
    auto getTrackCount() -> n7;
    auto getFirstTrack() -> n7;
    auto getLastTrack() -> n7;
    auto getCurrentTrack() -> n7;
    auto getCurrentSector() -> s32;
    auto getCurrentTimecode(u8& minute, u8& second, u8& frame) -> void;
    auto getCurrentTrackRelativeTimecode(u8& minute, u8& second, u8& frame) -> void;
    auto getLeadOutTimecode(u8& minute, u8& second, u8& frame) -> void;
    auto getTrackTocData(n7 track, u8& flags, u8& minute, u8& second, u8& frame) -> void;
    auto lbaFromTime(u8 hour, u8 minute, u8 second, u8 frame) -> s32;
    auto isTrackAudio(n7 track) -> bool;
    auto isDiscLoaded() -> bool;
    auto isDiscLaserdisc() -> bool;
    auto isLaserdiscClv() -> bool;
    auto isLaserdiscDigitalAudioPresent() -> bool;

    //serialization.cpp
    auto serialize(serializer&) -> void;

    Mode mode   = Mode::Inactive;  //current drive mode
    Mode seek   = Mode::Inactive;  //which mode to enter after seeking is completed
    u32 latency = 0;               //how many 75hz cycles are remaining in seek mode
    s32 lba     = 0;               //wnere the laser is currently at
    s32 start   = 0;               //where the laser should start reading
    s32 end     = 0;               //where the laser should stop reading
    n8  sector[2448];              //contains the most recently read disc sector
    n7  track;    //current track#
    i32 sectorRepeatCount;
    n1 stopPointEnabled;
    s32 targetStopPoint;
    bool laserdiscLoaded;
  } drive;

  struct SCSI {
    maybe<CD::Session&> session;
    maybe<Drive&> drive;
    maybe<CDDA&> cdda;
    maybe<ADPCM&> adpcm;
    enum class Status : u32 { OK, CheckCondition };

    //scsi.cpp
    auto clock() -> void;
    auto clockSector() -> void;
    auto clockSample() -> maybe<n8>;
    auto readData() -> n8;
    auto update() -> void;
    auto messageInput() -> void;
    auto messageOutput() -> void;
    auto dataInput() -> void;
    auto dataOutput() -> void;
    auto reply(Status) -> void;
    auto commandTestUnitReady() -> void;
    auto commandReadData() -> void;
    auto commandAudioSetStartPosition() -> void;
    auto commandAudioSetStopPosition() -> void;
    auto commandAudioPause() -> void;
    auto commandReadSubchannel() -> void;
    auto commandGetDirectoryInformation() -> void;
    auto commandInvalid() -> void;
    auto power() -> void;

    //serialization.cpp
    auto serialize(serializer&) -> void;

    struct IRQ {
      Interrupt ready;
      Interrupt completed;
    } irq;

    struct Pin {
      n1 reset;        //RST
      n1 acknowledge;  //ACK
      n1 select;       //SEL (1 = bus select requested)
      n1 input;        //I/O (1 = input, 0 = output)
      n1 control;      //C/D (1 = control, 0 = data)
      n1 message;      //MSG
      n1 request;      //REQ
      n1 busy;         //BSY
    } pin;

    struct Bus {
      n1 select;  //1 = bus is currently selected
      n8 data;    //D0-D7
    } bus;

    n8 acknowledging;
    n1 dataTransferCompleted;
    n1 messageAfterStatus;
    n1 messageSent;
    n1 statusSent;

    Buffer request;
    Buffer response;
  } scsi;

  struct CDDA {
    maybe<Drive&> drive;
    maybe<SCSI&> scsi;
    maybe<Fader&> fader;
    Node::Audio::Stream stream;

    //cdda.cpp
    auto load(Node::Object) -> void;
    auto unload(Node::Object) -> void;

    auto clockSector() -> void;
    auto clockSample() -> void;
    auto power() -> void;

    //serialization.cpp
    auto serialize(serializer&) -> void;

    enum class PlayMode : u32 { Loop, IRQ, Once } playMode = PlayMode::Loop;

    struct Sample {
      i16 left;
      i16 right;
      n12 offset;
    } sample;
  } cdda;

  struct ADPCM {
    maybe<SCSI&> scsi;
    maybe<Fader&> fader;
    MSM5205 msm5205;
    Node::Audio::Stream stream;
    Memory::Writable<n8> memory;  //64KB

    static constexpr u32 ReadLatency  = 20;  //estimation
    static constexpr u32 WriteLatency = 20;  //estimation

    //adpcm.cpp
    auto load(Node::Object) -> void;
    auto unload(Node::Object) -> void;

    auto clock() -> void;
    auto clockSample() -> void;
    auto control(n8 data) -> void;
    auto power() -> void;

    //serialization.cpp
    auto serialize(serializer&) -> void;

    struct IRQ {
      Interrupt halfReached;
      Interrupt endReached;
    } irq;

    struct IO {
      n1 writeOffset;
      n1 writeLatch;
      n1 readLatch;
      n1 readOffset;
      n1 lengthLatch;
      n1 playing;
      n1 noRepeat;  //0 = do not stop playing when length reaches zero
      n1 reset;
    } io;

    struct Bus {
      n16 address;
      n8  data;
      n8  pending;
    } read, write;

    struct Sample {
      n8 data;
      n1 nibble;
    } sample;

    n2  dmaActive;
    n8  divider;  //0x01-0x10
    n8  period;   //count-up for divider
    n16 latch;
    n16 length;
  } adpcm;

  struct Fader {
    enum class Mode : u32 { Idle, CDDA, ADPCM };

    auto cdda()  const -> f64 { return mode == Mode::CDDA  ? volume : 1.0; }
    auto adpcm() const -> f64 { return mode == Mode::ADPCM ? volume : 1.0; }

    //fader.cpp
    auto clock() -> void;
    auto power() -> void;

    //serialization.cpp
    auto serialize(serializer&) -> void;

    Mode mode;
    f64 step;
    f64 volume;
  } fader;

  struct Clock {
    n32 drive;
    n32 cdda;
    n32 adpcm;
    n32 fader;
  } clock;

  struct IO {
    n8  mdr[16];
    n1  cddaSampleSelect;
    n16 sramEnable;
    n1  bramEnable;
  } io;

  struct LD {
    // ldrom2.cpp
    auto load(string sourceFile) -> void;
    auto unload() -> void;
    auto notifyDiscEjected() -> void;
    auto read(n24 address) -> n8;
    auto write(n24 address, n8 data) -> void;
    auto getOutputRegisterValue(int regNum) -> n8;
    auto processInputRegisterWrite(int regNum, n8 data, n8 previousData, bool wasDeferredRegisterWrite) -> void;
    auto resetSeekTargetToDefault() -> void;
    auto liveSeekRegistersContainsLatchableTarget() const -> bool;
    auto latchSeekTargetFromCurrentState() -> bool;
    auto performSeekWithLatchedState() -> void;
    auto updateStopPointWithCurrentState() -> void;
    auto zeroBasedFrameIndexFromLba(s32 lba, bool processLeadIn = false) -> s32;
    auto lbaFromZeroBasedFrameIndex(s32 frameIndex) -> s32;
    auto VideoTimeToRedbookTime(u8& hours, u8& minutes, u8& seconds, u8& frames) -> void;
    auto handleStopPointReached(s32 lba) -> void;
    auto updateCurrentVideoFrameNumber(s32 lba) -> void;
    auto loadCurrentVideoFrameIntoBuffer() -> void;
    auto videoFramePrefetchThread() -> void;
    auto decodeBiphaseCodeFromScanline(int lineNo) -> u32;
    auto power() -> void;
    auto scanline(u32 vdpPixelBuffer[1128+48], u32 vcounter) -> void;

    //serialization.cpp
    auto serialize(serializer&) -> void;

    enum class SeekPointReg {
      Chapter,
      HoursOrFrameH,
      MinutesOrFrameM,
      SecondsOrFrameL,
      Frames,
    };
    enum class SeekMode {
      SeekToRedbookTime,
      SeekToRedbookRelativeTime,
      SeekToVideoFrame,
      SeekToVideoTime,
    };

    struct AnalogVideoFrameIndex {
      bool isCLV;
      bool hasDigitalAudio;
      size_t leadInFrameCount;
      size_t activeVideoFrameCount;
      size_t leadOutFrameCount;
      std::vector<const unsigned char*> leadInFrames;
      std::vector<const unsigned char*> activeVideoFrames;
      std::vector<const unsigned char*> leadOutFrames;

      s32 frameSkipBaseFrame;
      s32 frameSkipCounter;
      s32 currentVideoFrameIndex;
      n1 currentVideoFrameLeadIn;
      n1 currentVideoFrameLeadOut;
      n1 currentVideoFrameFieldSelectionEnabled[2];
      n1 currentVideoFrameFieldSelectionEvenField[2];
      n1 currentVideoFrameOnEvenField;
      n1 currentVideoFrameBlanked;
      n1 currentVideoFrameInterlaced;
      n1 digitalMemoryFrameLatched;
      qon_desc videoFileHeader;
      qoi2_desc videoFrameHeader;
      int drawIndex;
      std::vector<unsigned char> videoFrameBuffers[2];
      std::vector<unsigned char> dummyBlankLineBuffer;

      size_t vbiDataSearchStartPos;
      size_t vbiDataSearchEndPos;
      double vbiDataBitCellLengthInPixels;
      std::vector<size_t> vbiDataBitSampleOffsets;

      struct LineResamplingData {
        size_t firstSamplePosX;
        size_t lastSamplePosX;
        std::vector<float> sampleWeightX;
        float conversionFactor;
      };
      std::vector<LineResamplingData> lineResamplingData;

      static const size_t FrameBufferWidth = 1128 + 48;
      static const size_t FrameBufferHeight = 525;
      std::vector<u32> outputFramebuffer;
    } video;
    std::vector<u8> analogAudioDataBuffer;
    std::span<const u8> analogAudioRawDataView;
    n32 analogAudioLeadingAudioSamples;
    Decode::MMI mmi;

    static constexpr double videoFramesPerSecond = 30.0 / 1.001; // Roughly 29.97
    static const size_t inputRegisterCount = 0x20;
    static const size_t outputRegisterCount = 0x20;
    n8 inputRegs[inputRegisterCount];
    n8 inputFrozenRegs[inputRegisterCount];
    n8 outputRegs[outputRegisterCount];
    n8 outputFrozenRegs[outputRegisterCount];
    n8 outputRegsWrittenData[outputRegisterCount];
    n8 outputRegsWrittenCooldownTimer[outputRegisterCount];
    n1 areInputRegsFrozen;
    n1 areOutputRegsFrozen;
    n1 operationErrorFlag1;
    n1 operationErrorFlag2;
    n1 operationErrorFlag3;
    n1 seekEnabled;
    n3 currentSeekMode;
    n1 currentSeekModeTimeFormat;
    n1 currentSeekModeRepeat;
    n8 analogAudioAttenuationLeft;
    n8 analogAudioAttenuationRight;
    n1 analogAudioFadeToMutedLeft;
    n1 analogAudioFadeToMutedRight;

    // Currently latched seek point
    u8 activeSeekMode;
    n8 seekPointRegs[5];

    // Currently latched stop point
    n8 stopPointRegs[5];
    n1 reachedStopPoint;
    n1 reachedStopPointPreviously;

    u4 currentPlaybackMode;
    u3 currentPlaybackSpeed;
    u1 currentPlaybackDirection;
    n8 targetDriveState;
    n8 currentDriveState;
    n1 targetPauseState;
    n1 currentPauseState;
    n1 seekPerformedSinceLastFrameUpdate;
    n8 driveStateChangeDelayCounter;
    n8 selectedTrackInfo;

    // Prefetch thread state
    std::atomic_flag videoFramePrefetchPending;
    std::atomic_flag videoFramePrefetchComplete;
    std::atomic_flag videoFramePrefetchThreadStarted;
    std::atomic_flag videoFramePrefetchThreadShutdownRequested;
    std::atomic_flag videoFramePrefetchThreadShutdownComplete;
    const unsigned char* videoFramePrefetchTarget;
    std::vector<unsigned char> videoFramePrefetchBuffer;
  } ld;

  n1 sramEnable;

//unserialized:
  struct Information {
    string title;
  } information;
};

extern PCD pcd;
