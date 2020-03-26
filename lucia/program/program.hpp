struct Program : ares::Platform {
  auto create() -> void;
  auto main() -> void;
  auto quit() -> void;

  //platform.cpp
  auto attach(ares::Node::Object) -> void override;
  auto detach(ares::Node::Object) -> void override;
  auto open(ares::Node::Object, string name, vfs::file::mode mode, bool required) -> shared_pointer<vfs::file> override;
  auto event(ares::Event) -> void override;
  auto log(string_view message) -> void override;
  auto video(ares::Node::Screen, const uint32_t* data, uint pitch, uint width, uint height) -> void override;
  auto audio(ares::Node::Stream) -> void override;
  auto input(ares::Node::Input) -> void override;

  //load.cpp
  auto identify(const string& filename) -> shared_pointer<Emulator>;
  auto load(shared_pointer<Emulator> emulator, string filename = {}) -> bool;
  auto unload() -> void;

  //states.cpp
  auto stateSave(uint slot) -> bool;
  auto stateLoad(uint slot) -> bool;

  //status.cpp
  auto updateMessage() -> void;
  auto showMessage(const string&) -> void;

  //utility.cpp
  auto pause(bool) -> void;
  auto paletteUpdate() -> void;
  auto runAheadUpdate() -> void;
  auto captureScreenshot(const uint32_t* data, uint pitch, uint width, uint height) -> void;
  auto openFile(BrowserDialog&) -> string;
  auto selectFolder(BrowserDialog&) -> string;

  //drivers.cpp
  auto videoDriverUpdate() -> void;
  auto videoMonitorUpdate() -> void;
  auto videoFormatUpdate() -> void;
  auto videoFullScreenToggle() -> void;

  auto audioDriverUpdate() -> void;
  auto audioDeviceUpdate() -> void;
  auto audioFrequencyUpdate() -> void;
  auto audioLatencyUpdate() -> void;

  auto inputDriverUpdate() -> void;

  bool startFullScreen = false;
  string startGameLoad;

  vector<ares::Node::Screen> screens;
  vector<ares::Node::Stream> streams;

  bool paused = false;
  bool fastForwarding = false;
  bool rewinding = false;
  bool runAhead = false;
  bool requestScreenshot = false;

  struct State {
    uint slot = 1;
  } state;

  //rewind.cpp
  struct Rewind {
    enum class Mode : uint { Playing, Rewinding } mode = Mode::Playing;
    vector<serializer> history;
    uint length = 0;
    uint frequency = 0;
    uint counter = 0;
  } rewind;
  auto rewindSetMode(Rewind::Mode) -> void;
  auto rewindReset() -> void;
  auto rewindRun() -> void;

  struct Message {
    uint64_t timestamp = 0;
    string text;
  } message;
};

extern Program program;
