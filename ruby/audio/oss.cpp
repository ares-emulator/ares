#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/soundcard.h>

//OSSv4 features: define fallbacks for OSSv3 (where these ioctls are ignored)

#ifndef SNDCTL_DSP_COOKEDMODE
  #define SNDCTL_DSP_COOKEDMODE _IOW('P', 30, int)
#endif

#ifndef SNDCTL_DSP_POLICY
  #define SNDCTL_DSP_POLICY _IOW('P', 45, int)
#endif

struct AudioOSS : AudioDriver {
  AudioOSS& self = *this;
  AudioOSS(Audio& super) : AudioDriver(super) {}
  ~AudioOSS() { terminate(); }

  auto create() -> bool override {
    super.setDevice("/dev/dsp");
    super.setChannels(2);
    super.setFrequency(48000);
    super.setLatency(3);
    return initialize();
  }

  auto driver() -> string override { return "OSS"; }
  auto ready() -> bool override { return _fd >= 0; }

  auto hasBlocking() -> bool override { return true; }
  auto hasDynamic() -> bool override { return true; }

  auto hasDevices() -> vector<string> override {
    vector<string> devices;
    devices.append("/dev/dsp");
    for(auto& device : directory::files("/dev/", "dsp?*")) devices.append(string{"/dev/", device});
    return devices;
  }

  auto hasChannels() -> vector<u32> override {
    return {1, 2, 3, 4, 5, 6, 7, 8};
  }

  auto hasFrequencies() -> vector<u32> override {
    return {22050, 44100, 48000, 96000, 192000};
  }

  auto hasLatencies() -> vector<u32> override {
    return {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
  }

  auto setDevice(string device) -> bool override { return initialize(); }
  auto setBlocking(bool blocking) -> bool override { return updateBlocking(); }
  auto setChannels(u32 channels) -> bool override { return initialize(); }
  auto setFrequency(u32 frequency) -> bool override { return initialize(); }
  auto setLatency(u32 latency) -> bool override { return initialize(); }

  auto clear() -> void override {
    _buffer.fill();
  }

  auto level() -> double override {
    audio_buf_info info;
    ioctl(_fd, SNDCTL_DSP_GETOSPACE, &info);
    return (double)(_nonBlockBytes - info.bytes) / _nonBlockBytes;
  }

  auto output(const double samples[]) -> void override {
    if(!_buffer.capacity<u8>()) return;
    for(u32 n : range(self.channels)) {
      _buffer.write(sclamp<16>(samples[n] * 32767.0));
      if(_buffer.full()) {
        write(_fd, _buffer.data(), _buffer.capacity<u8>());
        _buffer.flush();
      }
    }
  }

private:
  auto initialize() -> bool {
    terminate();

    if(!hasDevices().find(self.device)) self.device = hasDevices().first();

    _fd = open(self.device, O_WRONLY | O_NONBLOCK);
    if(_fd < 0) return false;

    int cooked = 1;
    ioctl(_fd, SNDCTL_DSP_COOKEDMODE, &cooked);
    //policy: 0 = minimum latency (higher CPU usage); 10 = maximum latency (lower CPU usage)
    int policy = min(10, self.latency);
    ioctl(_fd, SNDCTL_DSP_POLICY, &policy);
    if(!updateChannels()) return terminate(), false;
    if(ioctl(_fd, SNDCTL_DSP_SETFMT, &_format) == -1) return terminate(), false;
    if(!updateFrequency()) return terminate(), false;
    if(!updateBlocking()) return terminate(), false;
    if(!updateNonBlockBytes()) return terminate(), false;

    _bufferSize = _frames * self.channels;
    _buffer.resize(_bufferSize);

    return true;
  }

  auto terminate() -> void {
    if(!ready()) return;

    _buffer.reset();

    close(_fd);
    _fd = -1;
  }

  auto updateChannels() -> bool {
    int channels = self.channels;
    if(ioctl(_fd, SNDCTL_DSP_CHANNELS, &channels) == -1) return false;
    if(!super.hasChannels(channels)) return false;
    super.updateResampleChannels(channels);
    self.channels = channels;
    return true;
  }

  auto updateFrequency() -> bool {
    int frequency = self.frequency;
    if(ioctl(_fd, SNDCTL_DSP_SPEED, &frequency) == -1) return false;
    if(!super.hasFrequency(frequency)) return false;
    super.updateResampleFrequency(frequency);
    self.frequency = frequency;
    return true;
  }

  auto updateBlocking() -> bool {
    if(!ready()) return false;
    auto flags = fcntl(_fd, F_GETFL);
    if(flags < 0) return false;
    self.blocking ? flags &=~ O_NONBLOCK : flags |= O_NONBLOCK;
    fcntl(_fd, F_SETFL, flags);
    return true;
  }

  auto updateNonBlockBytes() -> bool {
    audio_buf_info info;
    if(ioctl(_fd, SNDCTL_DSP_GETOSPACE, &info) == -1) return false;
    if(info.bytes < 1) return false;
    _nonBlockBytes = info.bytes;
    return true;
  }

  s32 _fd = -1;
  s32 _format = AFMT_S16_LE;
  static constexpr u32 _frames = 32;
  s32 _nonBlockBytes = 1;

  queue<s16> _buffer;
  u32 _bufferSize = 0;
};
