#if defined(MACOS_COMPILED_SDL)
#include "SDL.h"
#else
#include <SDL2/SDL.h>
#endif

struct AudioSDL : AudioDriver {
  AudioSDL& self = *this;
  AudioSDL(Audio& super) : AudioDriver(super) {}
  ~AudioSDL() { terminate(); }

  auto create() -> bool override {
    super.setChannels(2);
    super.setFrequency(48000);
    super.setLatency(0);
    return initialize();
  }

  auto driver() -> string override { return "SDL"; }
  auto ready() -> bool override { return _ready; }

  auto hasBlocking() -> bool override { return true; }
  auto hasDynamic() -> bool override { return true; }

  auto hasFrequencies() -> vector<u32> override {
    return {44100, 48000, 96000};
  }

  auto setFrequency(u32 frequency) -> bool override { return initialize(); }
  auto setLatency(u32 latency) -> bool override { return initialize(); }
  auto setBlocking(bool blocking) -> bool override { clear(); return true; }

  auto clear() -> void override {
    if(!ready()) return;
    SDL_ClearQueuedAudio(_device);
  }

  auto output(const f64 samples[]) -> void override {
    if(!ready()) return;

    if(self.blocking) {
      while(SDL_GetQueuedAudioSize(_device) > _bufferSize) {
        //wait for audio to drain
      }
    }

    std::unique_ptr<f32[]> output = std::make_unique<f32[]>(channels);
    for(auto n : range(channels)) output[n] = samples[n];
    SDL_QueueAudio(_device, &output[0], channels * sizeof(f32));
  }

  auto level() -> f64 override {
    return SDL_GetQueuedAudioSize(_device) / ((f64)_bufferSize);
  }

private:
  auto initialize() -> bool {
    terminate();

    SDL_InitSubSystem(SDL_INIT_AUDIO);

    SDL_AudioSpec want{}, have{};
    want.freq = frequency;
    want.format = AUDIO_F32SYS;
    want.channels = 2;
    want.samples = 4096;

    _device = SDL_OpenAudioDevice(NULL,0,&want,&have,0);
    frequency = have.freq;
    channels = have.channels;
    _bufferSize = have.size;
    SDL_PauseAudioDevice(_device, 0);

    _ready = true;
    clear();

    return true;
  }

  auto terminate() -> void {
    _ready = false;
    SDL_CloseAudioDevice(_device);
    SDL_QuitSubSystem(SDL_INIT_AUDIO);
  }

  bool _ready = false;

  SDL_AudioDeviceID _device = 0;
  u32 _bufferSize = 0;
};
