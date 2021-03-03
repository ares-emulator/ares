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
  //PC Engine Duo only:
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

private:
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
    auto seekRead() -> void;
    auto seekPlay() -> void;
    auto seekPause() -> void;
    auto read() -> bool;
    auto power() -> void;

    //serialization.cpp
    auto serialize(serializer&) -> void;

    Mode mode   = Mode::Inactive;  //current drive mode
    Mode seek   = Mode::Inactive;  //which mode to enter after seeking is completed
    u32 latency = 0;               //how many 75hz cycles are remaining in seek mode
    s32 lba     = 0;               //wnere the laser is currently at
    s32 start   = 0;               //where the laser should start reading
    s32 end     = 0;               //where the laser should stop reading
    n8  sector[2448];              //contains the most recently read disc sector
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

//unserialized:
  struct Information {
    string title;
  } information;
};

extern PCD pcd;
