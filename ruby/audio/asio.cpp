#include <nall/windows/registry.hpp>
#include "asio.hpp"

struct AudioASIO : AudioDriver {
  static AudioASIO* instance;
  AudioASIO& self = *this;
  AudioASIO(Audio& super) : AudioDriver(super) { instance = this; }
  ~AudioASIO() { terminate(); }

  auto create() -> bool override {
    super.setDevice(hasDevices().first());
    super.setChannels(2);
    super.setFrequency(48000);
    super.setLatency(2048);
    return initialize();
  }

  auto driver() -> string override { return "ASIO"; }
  auto ready() -> bool override { return _ready; }

  auto hasContext() -> bool override { return true; }
  auto hasBlocking() -> bool override { return true; }

  auto hasDevices() -> std::vector<string> override {
    self.devices.clear();
    for(auto candidate : registry::contents("HKLM\\SOFTWARE\\ASIO\\")) {
      if(auto classID = registry::read({"HKLM\\SOFTWARE\\ASIO\\", candidate, "CLSID"})) {
        self.devices.push_back({candidate.trimRight("\\", 1L), classID});
      }
    }

    std::vector<string> devices;
    for(auto& device : self.devices) devices.push_back(device.name);
    return devices;
  }

  auto hasChannels() -> std::vector<u32> override {
    return {1, 2};
  }

  auto hasFrequencies() -> std::vector<u32> override {
    return {self.frequency};
  }

  auto hasLatencies() -> std::vector<u32> override {
    std::vector<u32> latencies;
    u32 latencyList[] = {64, 96, 128, 192, 256, 384, 512, 768, 1024, 1536, 2048, 3072, 6144};  //factors of 6144
    for(auto& latency : latencyList) {
      if(self.activeDevice) {
        if(latency < self.activeDevice.minimumBufferSize) continue;
        if(latency > self.activeDevice.maximumBufferSize) continue;
      }
      latencies.push_back(latency);
    }
    //it is possible that no latencies in the hard-coded list above will match; so ensure driver-declared latencies are available
    if(std::ranges::find(latencies, self.activeDevice.minimumBufferSize) == latencies.end()) latencies.push_back(self.activeDevice.minimumBufferSize);
    if(std::ranges::find(latencies, self.activeDevice.maximumBufferSize) == latencies.end()) latencies.push_back(self.activeDevice.maximumBufferSize);
    if(std::ranges::find(latencies, self.activeDevice.preferredBufferSize) == latencies.end()) latencies.push_back(self.activeDevice.preferredBufferSize);
    latencies.sort();
    return latencies;
  }

  auto setContext(uintptr context) -> bool override { return initialize(); }
  auto setDevice(string device) -> bool override { return initialize(); }
  auto setBlocking(bool blocking) -> bool override { return initialize(); }
  auto setChannels(u32 channels) -> bool override { return initialize(); }
  auto setLatency(u32 latency) -> bool override { return initialize(); }

  auto clear() -> void override {
    if(!ready()) return;
    for(u32 n : range(self.channels)) {
      memory::fill<u8>(_channel[n].buffers[0], self.latency * _sampleSize);
      memory::fill<u8>(_channel[n].buffers[1], self.latency * _sampleSize);
    }
    memory::fill<u8>(_queue.samples, sizeof(_queue.samples));
    _queue.read = 0;
    _queue.write = 0;
    _queue.count = 0;
  }

  auto output(const f64 samples[]) -> void override {
    if(!ready()) return;
    //defer call to IASIO::start(), because the drivers themselves will sometimes crash internally.
    //if software initializes AudioASIO but does not play music at startup, this can prevent a crash loop.
    if(!_started) {
      _started = true;
      if(_asio->start() != ASE_OK) {
        _ready = false;
        return;
      }
    }
    if(self.blocking) {
      while(_queue.count >= self.latency);
    }
    for(u32 n : range(self.channels)) {
      _queue.samples[_queue.write][n] = samples[n];
    }
    _queue.write++;
    _queue.count++;
  }

private:
  auto initialize() -> bool {
    terminate();

    hasDevices();  //this call populates self.devices
    if(!self.devices) return false;

    self.activeDevice = {};
    for(auto& device : self.devices) {
      if(self.device == device.name) {
        self.activeDevice = device;
        break;
      }
    }
    if(!self.activeDevice) {
      self.activeDevice = self.devices.first();
      self.device = self.activeDevice.name;
    }

    CLSID classID;
    if(CLSIDFromString((LPOLESTR)utf16_t(self.activeDevice.classID), (LPCLSID)&classID) != S_OK) return false;
    if(CoCreateInstance(classID, 0, CLSCTX_INPROC_SERVER, classID, (void**)&_asio) != S_OK) return false;

    if(!_asio->init((void*)self.context)) return false;
    if(_asio->getSampleRate(&self.activeDevice.sampleRate) != ASE_OK) return false;
    if(_asio->getChannels(&self.activeDevice.inputChannels, &self.activeDevice.outputChannels) != ASE_OK) return false;
    if(_asio->getBufferSize(
      &self.activeDevice.minimumBufferSize,
      &self.activeDevice.maximumBufferSize,
      &self.activeDevice.preferredBufferSize,
      &self.activeDevice.granularity
    ) != ASE_OK) return false;

    self.frequency = self.activeDevice.sampleRate;
    self.latency = self.latency < self.activeDevice.minimumBufferSize ? self.activeDevice.minimumBufferSize : self.latency;
    self.latency = self.latency > self.activeDevice.maximumBufferSize ? self.activeDevice.maximumBufferSize : self.latency;

    for(u32 n : range(self.channels)) {
      _channel[n].isInput = false;
      _channel[n].channelNum = n;
      _channel[n].buffers[0] = nullptr;
      _channel[n].buffers[1] = nullptr;
    }
    ASIOCallbacks callbacks;
    callbacks.bufferSwitch = &AudioASIO::_bufferSwitch;
    callbacks.sampleRateDidChange = &AudioASIO::_sampleRateDidChange;
    callbacks.asioMessage = &AudioASIO::_asioMessage;
    callbacks.bufferSwitchTimeInfo = &AudioASIO::_bufferSwitchTimeInfo;
    if(_asio->createBuffers(_channel, self.channels, self.latency, &callbacks) != ASE_OK) return false;
    if(_asio->getLatencies(&self.activeDevice.inputLatency, &self.activeDevice.outputLatency) != ASE_OK) return false;

    //assume for the sake of sanity that all buffers use the same sample format ...
    ASIOChannelInfo channelInformation = {};
    channelInformation.channel = 0;
    channelInformation.isInput = false;
    if(_asio->getChannelInfo(&channelInformation) != ASE_OK) return false;
    switch(_sampleFormat = channelInformation.type) {
    case ASIOSTInt16LSB: _sampleSize = 2; break;
    case ASIOSTInt24LSB: _sampleSize = 3; break;
    case ASIOSTInt32LSB: _sampleSize = 4; break;
    case ASIOSTFloat32LSB: _sampleSize = 4; break;
    case ASIOSTFloat64LSB: _sampleSize = 8; break;
    default: return false;  //unsupported sample format
    }

    _ready = true;
    _started = false;
    clear();
    return true;
  }

  auto terminate() -> void {
    _ready = false;
    _started = false;
    self.activeDevice = {};
    if(_asio) {
      _asio->stop();
      _asio->disposeBuffers();
      _asio->Release();
      _asio = nullptr;
    }
  }

private:
  static auto _bufferSwitch(long doubleBufferInput, ASIOBool directProcess) -> void {
    return instance->bufferSwitch(doubleBufferInput, directProcess);
  }

  static auto _sampleRateDidChange(ASIOSampleRate sampleRate) -> void {
    return instance->sampleRateDidChange(sampleRate);
  }

  static auto _asioMessage(long selector, long value, void* message, double* optional) -> long {
    return instance->asioMessage(selector, value, message, optional);
  }

  static auto _bufferSwitchTimeInfo(ASIOTime* parameters, long doubleBufferIndex, ASIOBool directProcess) -> ASIOTime* {
    return instance->bufferSwitchTimeInfo(parameters, doubleBufferIndex, directProcess);
  }

  auto bufferSwitch(long doubleBufferInput, ASIOBool directProcess) -> void {
    for(u32 sampleIndex : range(self.latency)) {
      f64 samples[8] = {0};
      if(_queue.count) {
        for(u32 n : range(self.channels)) {
          samples[n] = _queue.samples[_queue.read][n];
        }
        _queue.read++;
        _queue.count--;
      }

      for(u32 n : range(self.channels)) {
        auto buffer = (u8*)_channel[n].buffers[doubleBufferInput];
        buffer += sampleIndex * _sampleSize;

        switch(_sampleFormat) {
        case ASIOSTInt16LSB: {
          *(u16*)buffer = (u16)sclamp<16>(samples[n] * (32768.0 - 1.0));
          break;
        }

        case ASIOSTInt24LSB: {
          auto value = (u32)sclamp<24>(samples[n] * (256.0 * 32768.0 - 1.0));
          buffer[0] = value >>  0;
          buffer[1] = value >>  8;
          buffer[2] = value >> 16;
          break;
        }

        case ASIOSTInt32LSB: {
          *(u32*)buffer = (u32)sclamp<32>(samples[n] * (65536.0 * 32768.0 - 1.0));
          break;
        }

        case ASIOSTFloat32LSB: {
          *(f32*)buffer = max(-1.0, min(+1.0, samples[n]));
          break;
        }

        case ASIOSTFloat64LSB: {
          *(f64*)buffer = max(-1.0, min(+1.0, samples[n]));
          break;
        }
        }
      }
    }
  }

  auto sampleRateDidChange(ASIOSampleRate sampleRate) -> void {
  }

  auto asioMessage(long selector, long value, void* message, double* optional) -> long {
    return ASE_OK;
  }

  auto bufferSwitchTimeInfo(ASIOTime* parameters, long doubleBufferIndex, ASIOBool directProcess) -> ASIOTime* {
    return nullptr;
  }

  bool _ready = false;
  bool _started = false;

  struct Queue {
    f64 samples[65536][8];
    u16 read;
    u16 write;
    std::atomic<u16> count;
  };

  struct Device {
    explicit operator bool() const { return name; }

    string name;
    string classID;

    ASIOSampleRate sampleRate;
    long inputChannels;
    long outputChannels;
    long inputLatency;
    long outputLatency;
    long minimumBufferSize;
    long maximumBufferSize;
    long preferredBufferSize;
    long granularity;
  };

  Queue _queue;
  std::vector<Device> devices;
  Device activeDevice;
  IASIO* _asio = nullptr;
  ASIOBufferInfo _channel[8];
  long _sampleFormat;
  long _sampleSize;
};

AudioASIO* AudioASIO::instance = nullptr;
