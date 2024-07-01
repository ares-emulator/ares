struct Program : ares::Platform {
  auto create() -> void;
  auto main() -> void;
  auto quit() -> void;

  //platform.cpp
  auto attach(ares::Node::Object) -> void override;
  auto detach(ares::Node::Object) -> void override;
  auto pak(ares::Node::Object) -> shared_pointer<vfs::directory> override;
  auto event(ares::Event) -> void override;
  auto log(ares::Node::Debugger::Tracer::Tracer tracer, string_view message) -> void override;
  auto status(string_view message) -> void override;
  auto video(ares::Node::Video::Screen, const u32* data, u32 pitch, u32 width, u32 height) -> void override;
  auto refreshRateHint(ares::Node::Video::Screen node, double refreshRate) -> void override;
  auto resetPalette(ares::Node::Video::Screen node) -> void override;
  auto resetSprites(ares::Node::Video::Screen node) -> void override;
  auto setViewport(ares::Node::Video::Screen node, u32 x, u32 y, u32 width, u32 height) -> void override;
  auto setOverscan(ares::Node::Video::Screen node, bool overscan) -> void override;
  auto setSize(ares::Node::Video::Screen node, u32 width, u32 height) -> void override;
  auto setScale(ares::Node::Video::Screen node, f64 scaleX, f64 scaleY) -> void override;
  auto setAspect(ares::Node::Video::Screen node, f64 aspectX, f64 aspectY) -> void override;
  auto setSaturation(ares::Node::Video::Screen node, f64 saturation) -> void override;
  auto setGamma(ares::Node::Video::Screen node, f64 gamma) -> void override;
  auto setLuminance(ares::Node::Video::Screen node, f64 luminance) -> void override;
  auto setFillColor(ares::Node::Video::Screen node, u32 fillColor) -> void override;
  auto setColorBleed(ares::Node::Video::Screen node, bool colorBleed) -> void override;
  auto setColorBleedWidth(ares::Node::Video::Screen node, u32 width) -> void override;
  auto setInterframeBlending(ares::Node::Video::Screen node, bool interframeBlending) -> void override;
  auto setRotation(ares::Node::Video::Screen node, u32 rotation) -> void override;
  auto setProgressive(ares::Node::Video::Screen node, bool progressiveDouble) -> void override;
  auto setInterlace(ares::Node::Video::Screen node, bool interlaceField) -> void override;
  auto attachSprite(ares::Node::Video::Screen node, ares::Node::Video::Sprite sprite) -> void override;
  auto detachSprite(ares::Node::Video::Screen node, ares::Node::Video::Sprite sprite) -> void override;
  auto colors(ares::Node::Video::Screen node, u32 colors, function<n64 (n32)> color) -> void override;
  auto audio(ares::Node::Audio::Stream) -> void override;
  auto input(ares::Node::Input::Input) -> void override;
  auto cheat(u32 address) -> maybe<u32> override;

  //load.cpp
  auto identify(const string& filename) -> shared_pointer<Emulator>;
  auto load(shared_pointer<Emulator> emulator, string location = {}) -> bool;
  auto load(string location) -> bool;
  auto unload() -> void;

  //states.cpp
  auto stateSave(u32 slot) -> bool;
  auto stateLoad(u32 slot) -> bool;
  auto undoStateSave() -> bool;
  auto undoStateLoad() -> bool;
  auto clearUndoStates() -> void;

  //status.cpp
  auto updateMessage() -> void;
  auto showMessage(const string&) -> void;

  //utility.cpp
  auto pause(bool) -> void;
  auto mute() -> void;
  auto paletteUpdate() -> void;
  auto runAheadUpdate() -> void;
  auto captureScreenshot(const u32* data, u32 pitch, u32 width, u32 height) -> void;
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
  vector<string> startGameLoad;
  bool noFilePrompt = false;

  string startSystem;
  string startShader;

  vector<ares::Node::Video::Screen> screens;
  vector<ares::Node::Audio::Stream> streams;

  bool paused = false;
  bool fastForwarding = false;
  bool rewinding = false;
  bool runAhead = false;
  bool requestFrameAdvance = false;
  bool requestScreenshot = false;
  bool keyboardCaptured = false;

  struct State {
    u32 slot = 1;
    u32 undoSlot = 1;
  } state;

  //rewind.cpp
  struct Rewind {
    enum class Mode : u32 { Playing, Rewinding } mode = Mode::Playing;
    vector<serializer> history;
    u32 length = 0;
    u32 frequency = 0;
    u32 counter = 0;
  } rewind;
  auto rewindSetMode(Rewind::Mode) -> void;
  auto rewindReset() -> void;
  auto rewindRun() -> void;

  struct Message {
    u64 timestamp = 0;
    string text;
  } message;

  vector<Message> messages;
  maybe<u64> vblanksPerSecond;
};

extern Program program;
