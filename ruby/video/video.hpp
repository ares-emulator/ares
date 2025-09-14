struct Video;

struct VideoDriver {
  VideoDriver(Video& super) : super(super) {}
  virtual ~VideoDriver() = default;

  virtual auto create() -> bool { return true; }
  virtual auto driver() -> string { return "None"; }
  virtual auto ready() -> bool { return true; }

  virtual auto hasFullScreen() -> bool { return false; }
  virtual auto hasMonitor() -> bool { return false; }
  virtual auto hasExclusive() -> bool { return false; }
  virtual auto hasContext() -> bool { return false; }
  virtual auto hasBlocking() -> bool { return false; }
  virtual auto hasForceSRGB() -> bool { return false; }
  virtual auto hasThreadedRenderer() -> bool { return false; }
  virtual auto hasNativeFullScreen() -> bool { return false; }
  virtual auto hasFlush() -> bool { return false; }
  virtual auto hasFormats() -> std::vector<string> { return {"ARGB24"}; }
  virtual auto hasShader() -> bool { return false; }

  auto hasFormat(string format) -> bool { 
    auto formats = hasFormats();
    return std::ranges::find(formats, format) != formats.end();
  }

  virtual auto setFullScreen(bool fullScreen) -> bool { return true; }
  virtual auto setMonitor(string monitor) -> bool { return true; }
  virtual auto setExclusive(bool exclusive) -> bool { return true; }
  virtual auto setContext(uintptr context) -> bool { return true; }
  virtual auto setBlocking(bool blocking) -> bool { return true; }
  virtual auto setForceSRGB(bool forceSRGB) -> bool { return true; }
  virtual auto setThreadedRenderer(bool threadedRenderer) -> bool { return true; }
  virtual auto setNativeFullScreen(bool nativeFullScreen) -> bool { return true; }
  virtual auto setFlush(bool flush) -> bool { return true; }
  virtual auto setFormat(string format) -> bool { return true; }
  virtual auto setShader(string shader) -> bool { return true; }
  virtual auto refreshRateHint(double refreshRate) -> void {}

  virtual auto focused() -> bool { return true; }
  virtual auto clear() -> void {}
  virtual auto size(u32& width, u32& height) -> void {}
  virtual auto acquire(u32*& data, u32& pitch, u32 width, u32 height) -> bool { return false; }
  virtual auto release() -> void {}
  virtual auto output(u32 width = 0, u32 height = 0) -> void {}
  virtual auto poll() -> void {}

protected:
  Video& super;
  friend struct Video;

  bool fullScreen = false;
  string monitor = "Primary";
  bool exclusive = false;
  uintptr context = 0;
  bool blocking = false;
  bool forceSRGB = false;
  bool threadedRenderer = true;
  bool nativeFullScreen = false;
  bool flush = false;
  string format = "ARGB24";
  string shader = "None";
};

struct Video {
  static auto hasDrivers() -> std::vector<string>;
  static auto hasDriver(string driver) -> bool { 
    auto drivers = hasDrivers(); 
    return std::ranges::find(drivers, driver) != drivers.end();
  }
  static auto optimalDriver() -> string;
  static auto safestDriver() -> string;

  struct Monitor {
    string name;
    bool primary = false;
    s32 x = 0;
    s32 y = 0;
    s32 width = 0;
    s32 height = 0;
    uintptr_t nativeHandle = 0;
  };
  static auto monitor(string name) -> Monitor;
  static auto hasMonitors() -> std::vector<Monitor>;
  static auto hasMonitor(string name) -> bool {
    for(auto& monitor : hasMonitors()) {
      if(monitor.name == name) return true;
    }
    return false;
  }

  Video() : self(*this) { reset(); }
  explicit operator bool() { return instance->driver() != "None"; }
  auto reset() -> void { instance = new VideoDriver(*this); }
  auto create(string driver = "") -> bool;
  auto driver() -> string { return instance->driver(); }
  auto ready() -> bool { return instance->ready(); }

  auto hasFullScreen() -> bool { return instance->hasFullScreen(); }
  auto hasMonitor() -> bool { return instance->hasMonitor(); }
  auto hasExclusive() -> bool { return instance->hasExclusive(); }
  auto hasContext() -> bool { return instance->hasContext(); }
  auto hasBlocking() -> bool { return instance->hasBlocking(); }
  auto hasForceSRGB() -> bool { return instance->hasForceSRGB(); }
  auto hasThreadedRenderer() -> bool { return instance->hasThreadedRenderer(); }
  auto hasNativeFullScreen() -> bool { return instance->hasNativeFullScreen(); }
  auto hasFlush() -> bool { return instance->hasFlush(); }
  auto hasFormats() -> std::vector<string> { return instance->hasFormats(); }
  auto hasShader() -> bool { return instance->hasShader(); }

  auto hasFormat(string format) -> bool { return instance->hasFormat(format); }

  auto fullScreen() -> bool { return instance->fullScreen; }
  auto monitor() -> string { return instance->monitor; }
  auto exclusive() -> bool { return instance->exclusive; }
  auto context() -> uintptr { return instance->context; }
  auto blocking() -> bool { return instance->blocking; }
  auto forceSRGB() -> bool { return instance->forceSRGB; }
  auto threadedRenderer() -> bool { return instance->threadedRenderer; }
  auto nativeFullScreen() -> bool { return instance->nativeFullScreen; }
  auto flush() -> bool { return instance->flush; }
  auto format() -> string { return instance->format; }
  auto shader() -> string { return instance->shader; }

  auto setMonitor(string monitor) -> bool;
  auto setFullScreen(bool fullScreen) -> bool;
  auto setExclusive(bool exclusive) -> bool;
  auto setContext(uintptr context) -> bool;
  auto setBlocking(bool blocking) -> bool;
  auto setForceSRGB(bool forceSRGB) -> bool;
  auto setThreadedRenderer(bool threadedRenderer) -> bool;
  auto setNativeFullScreen(bool nativeFullScreen) -> bool;
  auto setFlush(bool flush) -> bool;
  auto setFormat(string format) -> bool;
  auto setShader(string shader) -> bool;
  auto refreshRateHint(double refreshRate) -> void;

  auto focused() -> bool;
  auto clear() -> void;
  struct Size {
    u32 width = 0;
    u32 height = 0;
  };
  auto size() -> Size;
  struct Acquire {
    explicit operator bool() const { return data; }
    u32* data = nullptr;
    u32 pitch = 0;
  };
  auto acquire(u32 width, u32 height) -> Acquire;
  auto release() -> void;
  auto output(u32 width = 0, u32 height = 0) -> void;
  auto poll() -> void;

  auto onUpdate(const std::function<void (u32, u32)>&) -> void;
  auto doUpdate(u32 width, u32 height) -> void;

  auto lock() -> void { mutex.lock(); }
  auto unlock() -> void { mutex.unlock(); }

protected:
  Video& self;
  unique_pointer<VideoDriver> instance;
  std::function<void (u32, u32)> update;

private:
  std::recursive_mutex mutex;
};
