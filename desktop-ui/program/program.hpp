struct Program : ares::Platform {
  auto create() -> void;
  auto main() -> void;
  auto emulatorRunLoop(uintptr_t) -> void;
  auto quit() -> void;
  auto waitForInterrupts() -> void;

  //platform.cpp
  auto attach(ares::Node::Object) -> void override;
  auto detach(ares::Node::Object) -> void override;
  auto pak(ares::Node::Object) -> shared_pointer<vfs::directory> override;
  auto event(ares::Event) -> void override;
  auto log(ares::Node::Debugger::Tracer::Tracer tracer, string_view message) -> void override;
  auto status(string_view message) -> void override;
  auto video(ares::Node::Video::Screen, const u32* data, u32 pitch, u32 width, u32 height) -> void override;
  auto refreshRateHint(double refreshRate) -> void override;
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
  auto videoPseudoFullScreenToggle() -> void;

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

  class Guard;

  vector<Message> messages;
  string configuration;
  atomic<u64> vblanksPerSecond = 0;

  bool _isRunning = false;

  /// Mutex used to manage access to the input system. Polling occurs on the main thread while the results are read by the emulation thread.
  std::recursive_mutex inputMutex;
  
private:
  atomic<bool> _quitting = false;
  atomic<bool> _needsResize = false;
  
  /// Mutex used to manage access to the status message queue.
  std::recursive_mutex _messageMutex;

  /// Mutex used to manage interrupts to the emulator run loop. Acquired when setting either of the `_interruptWaiting` or `_interruptWorking` booleans.
  std::mutex _programMutex;
  /// The emulator run loop condition variable. This variable is signaled to indicate other threads may access the program mutex, as well as to indicate that the emulator run loop can continue.
  std::condition_variable _programConditionVariable;
  /// Boolean indicating whether another thread is waiting to modify the emulator program state.
  std::atomic<bool> _interruptWaiting = false;
  /// Boolean indicating whether a non-emulator worker thread is in the process of modifying the emulator program state.
  bool _interruptWorking = false;
  /// Counter used to allow for possible recursive creation of `Program::Guard` instances (which can happen in some UI paths).
  u32 _interruptDepth = 0;
  /// Thread-local variable used to allow the emulator worker thread to create `Program::Guard` instances without causing a deadlock.
  static inline thread_local bool _programThread = false;
  /// Debug accessor for `_programThread` since debuggers cannot read TLS values correctly.
  NALL_USED auto getProgramThreadValue() -> bool {
    return _programThread;
  }
};

extern Program program;
