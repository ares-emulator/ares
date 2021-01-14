#include <ao/ao.h>

struct AudioAO : AudioDriver {
  AudioAO& self = *this;
  AudioAO(Audio& super) : AudioDriver(super) {}
  ~AudioAO() { terminate(); }

  auto create() -> bool override {
    super.setChannels(2);
    super.setFrequency(48000);
    return initialize();
  }

  auto driver() -> string override { return "libao"; }
  auto ready() -> bool override { return _ready; }

  auto hasChannels() -> vector<u32> override {
    return {2};
  }

  auto hasFrequencies() -> vector<u32> override {
    return {44100, 48000, 96000};
  }

  auto setFrequency(u32 frequency) -> bool override { return initialize(); }

  auto output(const f64 samples[]) -> void override {
    u32 sample = 0;
    sample |= (u16)sclamp<16>(samples[0] * 32767.0) <<  0;
    sample |= (u16)sclamp<16>(samples[1] * 32767.0) << 16;
    ao_play(_interface, (char*)&sample, 4);
  }

private:
  auto initialize() -> bool {
    terminate();

    ao_initialize();

    s32 driverID = ao_default_driver_id();
    if(driverID < 0) return false;

    ao_sample_format format;
    format.bits = 16;
    format.channels = 2;
    format.rate = self.frequency;
    format.byte_format = AO_FMT_LITTLE;
    format.matrix = nullptr;

    ao_info* information = ao_driver_info(driverID);
    if(!information) return false;
    string device = information->short_name;

    ao_option* options = nullptr;
    if(device == "alsa") {
      ao_append_option(&options, "buffer_time", "100000");  //100ms latency (default was 500ms)
    }

    _interface = ao_open_live(driverID, &format, options);
    if(!_interface) return false;

    return _ready = true;
  }

  auto terminate() -> void {
    _ready = false;
    if(_interface) {
      ao_close(_interface);
      _interface = nullptr;
    }
    ao_shutdown();
  }

  bool _ready = false;

  ao_device* _interface = nullptr;
};
