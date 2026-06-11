#include "audio.hpp"

#include <SDL3/SDL.h>

namespace ruby {

auto Audio::create() -> bool {
  SDL_SetHint(SDL_HINT_NO_SIGNAL_HANDLERS, "1");
  return initialize();
}

auto Audio::hasDevices() -> std::vector<string> { 
  _devices.clear();
  _devices.push_back("Default");
  int count = 0;
  SDL_AudioDeviceID* deviceIds = SDL_GetAudioPlaybackDevices(&count);
  for(u32 i = 0; i < count; i++) {
    _devices.push_back(SDL_GetAudioDeviceName(deviceIds[i]));
  }
  SDL_free(deviceIds);
  return _devices; 
}

auto Audio::hasDevice(string device) -> bool {
    auto allDevices = hasDevices();
    return std::ranges::find(allDevices, device) != allDevices.end();
}

auto Audio::hasChannels(u32 channels) -> bool {
  auto allChannels = hasChannels();
  return std::ranges::find(allChannels, channels) != allChannels.end();
}

auto Audio::hasFrequency(u32 frequency) -> bool {
  auto allFrequencies = hasFrequencies();
  return std::ranges::find(allFrequencies, frequency) != allFrequencies.end();
}

auto Audio::hasLatency(u32 latency) -> bool {
  auto allLatencies = hasLatencies();
  return std::ranges::find(allLatencies, latency) != allLatencies.end();
}

auto Audio::setContext(uintptr context) -> bool {
  if(context == _context) return true;
  if(!hasContext()) return false;
  _context = context;
  return true;
}

auto Audio::setDevice(string device) -> bool {
  if(device == _device) return true;
  if(!hasDevice(device)) return false;
  _device = device;
  return true;
}

auto Audio::setBlocking(bool blocking) -> bool {
  if(blocking == _blocking) return true;
  if(!hasBlocking()) return false;
  _blocking = blocking;
  updateResampleFrequency(_frequency);
  clear();
  return true;
}

auto Audio::setDynamic(bool dynamic) -> bool {
  if(dynamic == _dynamic) return true;
  if(!hasDynamic()) return false;
  _dynamic = dynamic;
  return true;
}

auto Audio::setChannels(u32 channels) -> bool {
  updateResampleChannels(channels);
  if(channels == _channels) return true;
  if(!hasChannels(channels)) return false;
  _channels = channels;
  return true;
}

auto Audio::setFrequency(u32 frequency) -> bool {
  if(frequency == _frequency) return true;
  if(!hasFrequency(frequency)) return false;
  _frequency = frequency;
  updateResampleFrequency(_frequency);
  return initialize();
}

auto Audio::setLatency(u32 latency) -> bool {
  if(latency == _latency) return true;
  if(!hasLatency(latency)) return false;
  _latency = latency;
  return initialize();
}

auto Audio::clear() -> void{
  if(!_ready) return;
  SDL_ClearAudioStream(static_cast<SDL_AudioStream*>(_stream));
}

auto Audio::output(const f64 samples[]) -> void {
  if(!_ready) return;

  if(_blocking) {
    auto bytesRemaining = SDL_GetAudioStreamQueued(static_cast<SDL_AudioStream*>(_stream));
    while(bytesRemaining > _bufferSize) {
      // wait for audio to drain
      auto bytesToWait = (bytesRemaining - _bufferSize) * 4;
      auto framesRemaining = bytesToWait / _bytesPerFrame;
      auto secondsRemaining = framesRemaining / (f64)_frequency;
      usleep(max(1.0, secondsRemaining * 1000000.0));
      bytesRemaining = SDL_GetAudioStreamQueued(static_cast<SDL_AudioStream*>(_stream));
    }
  }

  f32 output[2];
  output[0] = samples[0];
  output[1] = samples[1];
  SDL_PutAudioStreamData(static_cast<SDL_AudioStream*>(_stream), output, sizeof(output));
}

auto Audio::level() -> f64 {
  if(_ready) return 0.0;
  return SDL_GetAudioStreamQueued(static_cast<SDL_AudioStream*>(_stream)) / ((f64)_bufferSize);
}

auto Audio::initialize() -> bool {
    terminate();

#if defined(PLATFORM_WINDOWS)
    timeBeginPeriod(1);
#endif

    if(!SDL_InitSubSystem(SDL_INIT_AUDIO)) return false;

    SDL_AudioSpec spec;
    spec.format = SDL_AUDIO_F32;
    spec.channels = 2;
    spec.freq = _frequency;

    auto desired_samples = (_latency * _frequency) / 1000;
    string desired_samples_string = (string)desired_samples;
    SDL_SetHint(SDL_HINT_AUDIO_DEVICE_SAMPLE_FRAMES, desired_samples_string);

    
    if(_device == "Default") {
      _deviceId = SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK;
    } else {
      s32 count = 0;
      SDL_AudioDeviceID* deviceIds = SDL_GetAudioPlaybackDevices(&count);
      for(auto i = 0; i < count; i++) {
        if(_device == SDL_GetAudioDeviceName(deviceIds[i])) _deviceId = deviceIds[i];
      }
      SDL_free(deviceIds);
    }

    _stream = SDL_OpenAudioDeviceStream(_deviceId, &spec, NULL, NULL);
    if(!_stream) return false;

    _deviceId = SDL_GetAudioStreamDevice(static_cast<SDL_AudioStream*>(_stream));
    if(!_deviceId) return false;

    SDL_ResumeAudioDevice(_deviceId);
    _frequency = spec.freq;
    _channels = spec.channels;

    _bytesPerFrame = SDL_AUDIO_FRAMESIZE(spec);
    _bufferSize = desired_samples * _bytesPerFrame;

    _ready = true;
    clear();

    return true;
  }

  auto Audio::terminate() -> void {
#if defined(PLATFORM_WINDOWS)
    timeEndPeriod(1);
#endif
    _ready = false;

    if(_stream) {
      SDL_DestroyAudioStream(static_cast<SDL_AudioStream*>(_stream));
      _stream = nullptr;
    } else if(_deviceId) {
      SDL_CloseAudioDevice(_deviceId);
    }

    _bufferSize = 0;
    _bytesPerFrame = 0;

    if(SDL_WasInit(SDL_INIT_AUDIO)) {
      SDL_QuitSubSystem(SDL_INIT_AUDIO);
    }
  }

auto Audio::updateResampleChannels(u32 channels) -> void {
  if(_resamplers.size() != channels) {
    _resamplers.clear();
    _resamplers.resize(channels);
    updateResampleFrequency(_frequency);
    _resampleBuffer.resize(channels);
  }
}

auto Audio::updateResampleFrequency(u32 frequency) -> void {
  for(auto& resampler : _resamplers) resampler.reset(frequency);
}

}
