#include <SDL3/SDL.h>

struct AudioSDL : AudioDriver {
  AudioSDL& self = *this;
  AudioSDL(Audio& super) : AudioDriver(super) {}
  ~AudioSDL() { terminate(); }

  auto create() -> bool override {
    SDL_SetHint(SDL_HINT_NO_SIGNAL_HANDLERS, "1");
    super.setChannels(2);
    super.setFrequency(48000);
    super.setLatency(40);
    return initialize();
  }

  auto driver() -> string override { return "SDL"; }
  auto ready() -> bool override { return _ready; }

  auto hasExclusive() -> bool override { return false; }
  auto hasBlocking() -> bool override { return true; }
  auto hasDynamic() -> bool override { return true; }

  auto hasFrequencies() -> std::vector<u32> override {
    return {22050, 44100, 48000, 96000};
  }

  auto hasLatencies() -> std::vector<u32> override {
    return {10, 20, 40, 60, 80};
  }

  auto setFrequency(u32 frequency) -> bool override { return initialize(); }
  auto setLatency(u32 latency) -> bool override { return initialize(); }
  auto setBlocking(bool blocking) -> bool override { clear(); return true; }

  auto clear() -> void override {
    if(!ready()) return;
    SDL_ClearAudioStream(_stream);
  }

  auto output(const f64 samples[]) -> void override {
    if(!ready()) return;

    if(self.blocking) {
      auto bytesRemaining = SDL_GetAudioStreamQueued(_stream);
      while(bytesRemaining > _bufferSize) {
        // wait for audio to drain
        auto bytesToWait = (bytesRemaining - _bufferSize) * 4;
        auto framesRemaining = bytesToWait / _bytesPerFrame;
        auto secondsRemaining = framesRemaining / (f64)frequency;
        usleep(max(1.0, secondsRemaining * 1000000.0));
        bytesRemaining = SDL_GetAudioStreamQueued(_stream);
      }
    }

    f32 output[2];
    output[0] = samples[0];
    output[1] = samples[1];
    SDL_PutAudioStreamData(_stream, output, sizeof(output));
  }

  auto level() -> f64 override {
    if(!ready()) return 0.0;
    return SDL_GetAudioStreamQueued(_stream) / ((f64)_bufferSize);
  }

private:
  auto initialize() -> bool {
    terminate();

#if defined(PLATFORM_WINDOWS)
    timeBeginPeriod(1);
#endif

    if(!SDL_InitSubSystem(SDL_INIT_AUDIO)) return false;

    SDL_AudioSpec spec;
    spec.format = SDL_AUDIO_F32;
    spec.channels = 2;
    spec.freq = frequency;

    auto desired_samples = (latency * frequency) / 1000;
    string desired_samples_string = (string)desired_samples;
    SDL_SetHint(SDL_HINT_AUDIO_DEVICE_SAMPLE_FRAMES, desired_samples_string);

    _stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, NULL, NULL);
    if(!_stream) return false;

    _device = SDL_GetAudioStreamDevice(_stream);
    if(!_device) return false;

    SDL_ResumeAudioDevice(_device);
    frequency = spec.freq;
    channels = spec.channels;

    _bytesPerFrame = SDL_AUDIO_FRAMESIZE(spec);
    _bufferSize = desired_samples * _bytesPerFrame;

    _ready = true;
    clear();

    return true;
  }

  auto terminate() -> void {
#if defined(PLATFORM_WINDOWS)
    timeEndPeriod(1);
#endif
    _ready = false;

    if(_stream) {
      SDL_DestroyAudioStream(_stream);
      _stream = nullptr;
    } else if(_device) {
      SDL_CloseAudioDevice(_device);
    }

    _device = 0;
    _bufferSize = 0;
    _bytesPerFrame = 0;

    if(SDL_WasInit(SDL_INIT_AUDIO)) {
      SDL_QuitSubSystem(SDL_INIT_AUDIO);
    }
  }

  bool _ready = false;

  SDL_AudioDeviceID _device = 0;
  SDL_AudioStream *_stream = nullptr;
  u32 _bufferSize = 0;
  u32 _bytesPerFrame = 0;
};
