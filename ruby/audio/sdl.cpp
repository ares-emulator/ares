#include <SDL3/SDL.h>

struct AudioSDL : AudioDriver {
  AudioSDL& self = *this;
  AudioSDL(Audio& super) : AudioDriver(super) {}
  ~AudioSDL() { terminate(); }

  auto create() -> bool override {
    super.setChannels(2);
    super.setFrequency(48000);
    super.setLatency(20);
    return initialize();
  }

  auto driver() -> string override { return "SDL"; }
  auto ready() -> bool override { return _ready; }

  auto hasBlocking() -> bool override { return true; }
  auto hasDynamic() -> bool override { return true; }
  
  double bitsPerSample = 0;

  auto hasFrequencies() -> std::vector<u32> override {
    return {44100, 48000, 96000};
  }

  auto hasLatencies() -> std::vector<u32> override {
    return {10, 20, 40, 60, 80, 100};
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
      auto bytesRemaining = SDL_GetAudioStreamAvailable(_stream);
      while(bytesRemaining > _bufferSize) {
        //wait for audio to drain
        auto bytesToWait = bytesRemaining - _bufferSize;
        auto bytesPerSample = bitsPerSample / 8.0;
        auto samplesRemaining = bytesToWait / bytesPerSample;
        auto secondsRemaining = samplesRemaining / frequency;
        usleep(secondsRemaining * 1000000);
        bytesRemaining = SDL_GetAudioStreamAvailable(_stream);
      }
    }

    std::unique_ptr<f32[]> output = std::make_unique<f32[]>(channels);
    for(auto n : range(channels)) output[n] = samples[n];
    SDL_PutAudioStreamData(_stream, &output[0], channels * sizeof(f32));
  }

  auto level() -> f64 override {
    return SDL_GetAudioStreamAvailable(_stream) / ((f64)_bufferSize);
  }

private:
  auto initialize() -> bool {
    terminate();
    
#if defined(PLATFORM_WINDOWS)
    timeBeginPeriod(1);
#endif

    SDL_InitSubSystem(SDL_INIT_AUDIO);

    SDL_AudioSpec spec;
    spec.format = SDL_AUDIO_F32;
    spec.channels = 2;
    spec.freq = frequency;
    auto desired_samples = (latency * frequency) / 1000.0f;
    string desired_samples_string = (string)desired_samples;
    SDL_SetHint(SDL_HINT_AUDIO_DEVICE_SAMPLE_FRAMES, desired_samples_string);
    
    SDL_AudioStream *stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, NULL, NULL);
    _device = SDL_GetAudioStreamDevice(stream);
    SDL_ResumeAudioDevice(_device);
    _stream = stream;
    frequency = spec.freq;
    channels = spec.channels;
    int bufferFrameSize;
    SDL_GetAudioDeviceFormat(_device, &spec, &bufferFrameSize);
    bitsPerSample = SDL_AUDIO_BITSIZE(spec.format);
    _bufferSize = bufferFrameSize * channels * 4;

    _ready = true;
    clear();

    return true;
  }

  auto terminate() -> void {
#if defined(PLATFORM_WINDOWS)
    timeEndPeriod(1);
#endif
    _ready = false;
    SDL_CloseAudioDevice(_device);
    SDL_QuitSubSystem(SDL_INIT_AUDIO);
  }

  bool _ready = false;

  SDL_AudioDeviceID _device = 0;
  SDL_AudioStream *_stream;
  u32 _bufferSize = 0;
};
