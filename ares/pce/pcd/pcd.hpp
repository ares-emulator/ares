//PC Engine CD-ROM

struct PCD : Thread {
  Node::Component node;
  Node::Port tray;
  Node::Peripheral disc;
  Shared::File fd;
  CD::Session session;
  Memory::Writable<uint8> wram;  // 64KB
  Memory::Writable<uint8> bram;  //  2KB
  //PC Engine Duo only:
  Memory::Readable<uint8> bios;  //256KB
  Memory::Writable<uint8> sram;  //192KB

  struct Debugger {
    //debugger.cpp
    auto load(Node::Object) -> void;

    struct Memory {
      Node::Memory wram;
      Node::Memory bram;
      Node::Memory sram;
      Node::Memory adpcm;
    } memory;
  } debugger;

  static auto Present() -> bool { return true; }

  auto manifest() const -> string { return information.manifest; }
  auto name() const -> string { return information.name; }
  auto sramEnable() const -> bool { return io.sramEnable == 0xaa55; }
  auto bramEnable() const -> bool { return io.bramEnable; }

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
  auto read(uint8 bank, uint13 address, uint8 data) -> uint8;
  auto write(uint8 bank, uint13 address, uint8 data) -> void;

  auto readIO(uint13 address, uint8 data) -> uint8;
  auto writeIO(uint13 address, uint8 data) -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

private:
  struct Interrupt {
    auto poll() const -> bool { return line & enable; }
    auto raise() -> void { line = 1; }
    auto lower() -> void { line = 0; }

    uint1 enable;
    uint1 line;
  };

  struct Buffer {
    auto reset() -> void { reads = 0; writes = 0; }
    auto end() const -> bool { return reads >= writes; }
    auto read() -> uint8 { return data[reads++]; }
    auto write(uint8 value) -> void { data[writes++] = value; }

     uint8 data[4_KiB];
    uint12 reads;
    uint12 writes;
  };

  struct Drive;
  struct SCSI;
  struct CDDA;
  struct ADPCM;
  struct Fader;

  struct Drive {
    maybe<CD::Session&> session;

    enum class Mode : uint {
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
    auto distance() -> uint;
    auto seekRead() -> void;
    auto seekPlay() -> void;
    auto seekPause() -> void;
    auto read() -> bool;
    auto power() -> void;

    //serialization.cpp
    auto serialize(serializer&) -> void;

    Mode mode    = Mode::Inactive;  //current drive mode
    Mode seek    = Mode::Inactive;  //which mode to enter after seeking is completed
    uint latency = 0;               //how many 75hz cycles are remaining in seek mode
    int lba      = 0;               //wnere the laser is currently at
    int start    = 0;               //where the laser should start reading
    int end      = 0;               //where the laser should stop reading
    uint8 sector[2448];             //contains the most recently read disc sector
  } drive;

  struct SCSI {
    maybe<CD::Session&> session;
    maybe<Drive&> drive;
    maybe<CDDA&> cdda;
    maybe<ADPCM&> adpcm;
    enum class Status : uint { OK, CheckCondition };

    //scsi.cpp
    auto clock() -> void;
    auto clockSector() -> void;
    auto clockSample() -> maybe<uint8>;
    auto readData() -> uint8;
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
      uint1 reset;        //RST
      uint1 acknowledge;  //ACK
      uint1 select;       //SEL (1 = bus select requested)
      uint1 input;        //I/O (1 = input, 0 = output)
      uint1 control;      //C/D (1 = control, 0 = data)
      uint1 message;      //MSG
      uint1 request;      //REQ
      uint1 busy;         //BSY
    } pin;

    struct Bus {
      uint1 select;  //1 = bus is currently selected
      uint8 data;    //D0-D7
    } bus;

    uint8 acknowledging;
    uint1 dataTransferCompleted;
    uint1 messageAfterStatus;
    uint1 messageSent;
    uint1 statusSent;

    Buffer request;
    Buffer response;
  } scsi;

  struct CDDA {
    maybe<Drive&> drive;
    maybe<SCSI&> scsi;
    maybe<Fader&> fader;
    Node::Stream stream;

    //cdda.cpp
    auto load(Node::Object) -> void;
    auto unload() -> void;

    auto clockSector() -> void;
    auto clockSample() -> void;
    auto power() -> void;

    //serialization.cpp
    auto serialize(serializer&) -> void;

    enum class PlayMode : uint { Loop, IRQ, Once } playMode = PlayMode::Loop;

    struct Sample {
       int16 left;
       int16 right;
      uint12 offset;
    } sample;
  } cdda;

  struct ADPCM {
    maybe<SCSI&> scsi;
    maybe<Fader&> fader;
    MSM5205 msm5205;
    Node::Stream stream;
    Memory::Writable<uint8> memory;  //64KB

    static constexpr uint ReadLatency  = 20;  //estimation
    static constexpr uint WriteLatency = 20;  //estimation

    //adpcm.cpp
    auto load(Node::Object) -> void;
    auto unload() -> void;

    auto clock() -> void;
    auto clockSample() -> void;
    auto control(uint8 data) -> void;
    auto power() -> void;

    //serialization.cpp
    auto serialize(serializer&) -> void;

    struct IRQ {
      Interrupt halfReached;
      Interrupt endReached;
    } irq;

    struct IO {
      uint1 writeOffset;
      uint1 writeLatch;
      uint1 readLatch;
      uint1 readOffset;
      uint1 lengthLatch;
      uint1 playing;
      uint1 noRepeat;  //0 = do not stop playing when length reaches zero
      uint1 reset;
    } io;

    struct Bus {
      uint16 address;
       uint8 data;
       uint8 pending;
    } read, write;

    struct Sample {
      uint8 data;
      uint1 nibble;
    } sample;

     uint2 dmaActive;
     uint8 divider;  //0x01-0x10
     uint8 period;   //count-up for divider
    uint16 latch;
    uint16 length;
  } adpcm;

  struct Fader {
    enum class Mode : uint { Idle, CDDA, ADPCM };

    auto cdda()  const -> double { return mode == Mode::CDDA  ? volume : 1.0; }
    auto adpcm() const -> double { return mode == Mode::ADPCM ? volume : 1.0; }

    //fader.cpp
    auto clock() -> void;
    auto power() -> void;

    //serialization.cpp
    auto serialize(serializer&) -> void;

    Mode mode;
    double step;
    double volume;
  } fader;

  struct Clock {
    uint32 drive;
    uint32 cdda;
    uint32 adpcm;
    uint32 fader;
  } clock;

  struct IO {
     uint8 mdr[16];
     uint1 cddaSampleSelect;
    uint16 sramEnable;
     uint1 bramEnable;
  } io;

//unserialized:
  struct Information {
    string manifest;
    string name;
  } information;
};

extern PCD pcd;
