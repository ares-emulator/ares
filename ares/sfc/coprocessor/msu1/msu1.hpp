struct MSU1 : Thread {
  Node::Audio::Stream stream;
  VFS::File dataFile;
  VFS::File audioFile;

  auto load(Node::Object) -> void;
  auto unload(Node::Object) -> void;

  auto main() -> void;
  auto power() -> void;

  auto dataOpen() -> void;
  auto audioOpen() -> void;

  auto readIO(n24 address, n8 data) -> n8;
  auto writeIO(n24 address, n8 data) -> void;

  auto serialize(serializer&) -> void;

private:
  enum Flag : u32 {
    Revision       = 0x02,  //max: 0x07
    AudioError     = 0x08,
    AudioPlaying   = 0x10,
    AudioRepeating = 0x20,
    AudioBusy      = 0x40,
    DataBusy       = 0x80,
  };

  struct IO {
    n32 dataSeekOffset;
    n32 dataReadOffset;

    n32 audioPlayOffset;
    n32 audioLoopOffset;

    n16 audioTrack;
    n8  audioVolume;

    n32 audioResumeTrack;
    n32 audioResumeOffset;

    n1  audioError;
    n1  audioPlay;
    n1  audioRepeat;
    n1  audioBusy;
    n1  dataBusy;
  } io;
};

extern MSU1 msu1;
