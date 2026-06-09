#pragma once

struct Audio {
  ~Audio() { terminate(); }
  auto create() -> bool;
  auto driver() -> string { return "SDL"; }
  auto ready() -> bool { return _ready; }

  auto hasContext() -> bool { return false; }
  auto hasDevices() -> std::vector<string> { return {"Default"}; }
  auto hasBlocking() -> bool { return true; }
  auto hasDynamic() -> bool { return true; }
  auto hasChannels() -> std::vector<u32> { return { _channels }; }
  auto hasFrequencies() -> std::vector<u32> { return {22050, 44100, 48000, 96000}; }
  auto hasLatencies() -> std::vector<u32> { return {10, 20, 40, 60, 80}; }

  auto hasDevice(string device) -> bool;
  auto hasChannels(u32 channels) -> bool;
  auto hasFrequency(u32 frequency) -> bool;
  auto hasLatency(u32 latency) -> bool;

  auto context() -> uintptr { return _context; }
  auto device() -> string { return _device; }
  auto blocking() -> bool { return _blocking; }
  auto dynamic() -> bool { return _dynamic; }
  auto channels() -> u32 { return _channels; }
  auto frequency() -> u32 { return _frequency; }
  auto latency() -> u32 { return _latency; }

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

private:
  auto initialize() -> bool;
  auto terminate() -> void;

  auto updateResampleChannels(u32 channels) -> void;
  auto updateResampleFrequency(u32 frequency) -> void;

  bool _ready = false;
  bool _blocking = false;
  bool _dynamic = false;

  u32 _deviceId = 0;
  u32 _bufferSize = 0;
  u32 _bytesPerFrame = 0;
  u32 _channels = 2;
  u32 _frequency = 48000;
  u32 _latency = 40;
  
  void* _stream = nullptr;
  uintptr _context = 0;
  string _device = "Default";

  std::vector<nall::DSP::Resampler::Cubic> _resamplers;
  std::vector<f64> _resampleBuffer;
};
