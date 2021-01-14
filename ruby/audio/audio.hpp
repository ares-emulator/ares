struct Audio;

struct AudioDriver {
  enum class Format : u32 { none, int16, int32, float32, float64 };

  AudioDriver(Audio& super) : super(super) {}
  virtual ~AudioDriver() = default;

  virtual auto create() -> bool { return true; }
  virtual auto driver() -> string { return "None"; }
  virtual auto ready() -> bool { return true; }

  virtual auto hasExclusive() -> bool { return false; }
  virtual auto hasContext() -> bool { return false; }
  virtual auto hasDevices() -> vector<string> { return {"Default"}; }
  virtual auto hasBlocking() -> bool { return false; }
  virtual auto hasDynamic() -> bool { return false; }
  virtual auto hasChannels() -> vector<u32> { return {2}; }
  virtual auto hasFrequencies() -> vector<u32> { return {48000}; }
  virtual auto hasLatencies() -> vector<u32> { return {0}; }

  auto hasDevice(string device) -> bool { return (bool)hasDevices().find(device); }
  auto hasChannels(u32 channels) -> bool { return (bool)hasChannels().find(channels); }
  auto hasFrequency(u32 frequency) -> bool { return (bool)hasFrequencies().find(frequency); }
  auto hasLatency(u32 latency) -> bool { return (bool)hasLatencies().find(latency); }

  virtual auto setExclusive(bool exclusive) -> bool { return true; }
  virtual auto setContext(uintptr context) -> bool { return true; }
  virtual auto setDevice(string device) -> bool { return true; }
  virtual auto setBlocking(bool blocking) -> bool { return true; }
  virtual auto setDynamic(bool dynamic) -> bool { return true; }
  virtual auto setChannels(u32 channels) -> bool { return true; }
  virtual auto setFrequency(u32 frequency) -> bool { return true; }
  virtual auto setLatency(u32 latency) -> bool { return true; }

  virtual auto clear() -> void {}
  virtual auto level() -> f64 { return 0.5; }
  virtual auto output(const f64 samples[]) -> void {}

protected:
  Audio& super;
  friend class Audio;

  bool exclusive = false;
  uintptr context = 0;
  string device = "Default";
  bool blocking = false;
  bool dynamic = false;
  u32 channels = 2;
  u32 frequency = 48000;
  u32 latency = 0;
};

struct Audio {
  static auto hasDrivers() -> vector<string>;
  static auto hasDriver(string driver) -> bool { return (bool)hasDrivers().find(driver); }
  static auto optimalDriver() -> string;
  static auto safestDriver() -> string;

  Audio() : self(*this) { reset(); }
  explicit operator bool() { return instance->driver() != "None"; }
  auto reset() -> void { instance = new AudioDriver(*this); }
  auto create(string driver = "") -> bool;
  auto driver() -> string { return instance->driver(); }
  auto ready() -> bool { return instance->ready(); }

  auto hasExclusive() -> bool { return instance->hasExclusive(); }
  auto hasContext() -> bool { return instance->hasContext(); }
  auto hasDevices() -> vector<string> { return instance->hasDevices(); }
  auto hasBlocking() -> bool { return instance->hasBlocking(); }
  auto hasDynamic() -> bool { return instance->hasDynamic(); }
  auto hasChannels() -> vector<u32> { return instance->hasChannels(); }
  auto hasFrequencies() -> vector<u32> { return instance->hasFrequencies(); }
  auto hasLatencies() -> vector<u32> { return instance->hasLatencies(); }

  auto hasDevice(string device) -> bool { return instance->hasDevice(device); }
  auto hasChannels(u32 channels) -> bool { return instance->hasChannels(channels); }
  auto hasFrequency(u32 frequency) -> bool { return instance->hasFrequency(frequency); }
  auto hasLatency(u32 latency) -> bool { return instance->hasLatency(latency); }

  auto exclusive() -> bool { return instance->exclusive; }
  auto context() -> uintptr { return instance->context; }
  auto device() -> string { return instance->device; }
  auto blocking() -> bool { return instance->blocking; }
  auto dynamic() -> bool { return instance->dynamic; }
  auto channels() -> u32 { return instance->channels; }
  auto frequency() -> u32 { return instance->frequency; }
  auto latency() -> u32 { return instance->latency; }

  auto setExclusive(bool exclusive) -> bool;
  auto setContext(uintptr context) -> bool;
  auto setDevice(string device) -> bool;
  auto setBlocking(bool blocking) -> bool;
  auto setDynamic(bool dynamic) -> bool;
  auto setChannels(u32 channels) -> bool;
  auto setFrequency(u32 frequency) -> bool;
  auto setLatency(u32 latency) -> bool;

  auto clear() -> void;
  auto level() -> double;
  auto output(const f64 samples[]) -> void;

protected:
  Audio& self;
  unique_pointer<AudioDriver> instance;
  vector<nall::DSP::Resampler::Cubic> resamplers;
};
