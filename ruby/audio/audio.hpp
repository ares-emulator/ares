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
  virtual auto hasDevices() -> std::vector<string> { return {"Default"}; }
  virtual auto hasBlocking() -> bool { return false; }
  virtual auto hasDynamic() -> bool { return false; }
  virtual auto hasChannels() -> std::vector<u32> { return {2}; }
  virtual auto hasFrequencies() -> std::vector<u32> { return {48000}; }
  virtual auto hasLatencies() -> std::vector<u32> { return {0}; }

  auto hasDevice(string device) -> bool {
    auto allDevices = hasDevices();
    return std::ranges::find(allDevices, device) != allDevices.end();
  }
  auto hasChannels(u32 channels) -> bool {
    auto allChannels = hasChannels();
    return std::ranges::find(allChannels, channels) != allChannels.end();
  }
  auto hasFrequency(u32 frequency) -> bool {
    auto allFrequencies = hasFrequencies();
    return std::ranges::find(allFrequencies, frequency) != allFrequencies.end();
  }
  auto hasLatency(u32 latency) -> bool {
    auto allLatencies = hasLatencies();
    return std::ranges::find(allLatencies, latency) != allLatencies.end();
  }

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
  friend struct Audio;

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
  static auto hasDrivers() -> std::vector<string>;
  static auto hasDriver(string driver) -> bool {
    auto drivers = hasDrivers(); 
    return std::ranges::find(drivers, driver) != drivers.end();
  }
  static auto optimalDriver() -> string;
  static auto safestDriver() -> string;

  Audio() : self(*this) { reset(); }
  explicit operator bool() { return instance->driver() != "None"; }
  auto reset() -> void { instance = std::make_unique<AudioDriver>(*this); }
  auto create(string driver = "") -> bool;
  auto driver() -> string { return instance->driver(); }
  auto ready() -> bool { return instance->ready(); }

  auto hasExclusive() -> bool { return instance->hasExclusive(); }
  auto hasContext() -> bool { return instance->hasContext(); }
  auto hasDevices() -> std::vector<string> { return instance->hasDevices(); }
  auto hasBlocking() -> bool { return instance->hasBlocking(); }
  auto hasDynamic() -> bool { return instance->hasDynamic(); }
  auto hasChannels() -> std::vector<u32> { return instance->hasChannels(); }
  auto hasFrequencies() -> std::vector<u32> { return instance->hasFrequencies(); }
  auto hasLatencies() -> std::vector<u32> { return instance->hasLatencies(); }

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

  auto updateResampleChannels(u32 channels) -> void;
  auto updateResampleFrequency(u32 frequency) -> void;

  auto clear() -> void;
  auto level() -> double;
  auto output(const f64 samples[]) -> void;

protected:
  Audio& self;
  std::unique_ptr<AudioDriver> instance;
  std::vector<nall::DSP::Resampler::Cubic> resamplers;
  std::vector<f64> resampleBuffer;
};
