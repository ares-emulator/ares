#if defined(AUDIO_ALSA)
  #include <ruby/audio/alsa.cpp>
#endif

#if defined(AUDIO_AO)
  #include <ruby/audio/ao.cpp>
#endif

#if defined(AUDIO_ASIO)
  #include <ruby/audio/asio.cpp>
#endif

#if defined(AUDIO_DIRECTSOUND)
  #include <ruby/audio/directsound.cpp>
#endif

#if defined(AUDIO_OPENAL)
  #if defined(__APPLE__)
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wdeprecated-declarations"
  #endif

  #include <ruby/audio/openal.cpp>

  #if defined(__APPLE__)
    #pragma clang diagnostic pop
  #endif
#endif

#if defined(AUDIO_OSS)
  #include <ruby/audio/oss.cpp>
#endif

#if defined(AUDIO_PULSEAUDIO)
  #include <ruby/audio/pulseaudio.cpp>
#endif

#if defined(AUDIO_PULSEAUDIOSIMPLE)
  #include <ruby/audio/pulseaudio-simple.cpp>
#endif

#if defined(AUDIO_WASAPI)
  #include <ruby/audio/wasapi.cpp>
#endif

#if defined(AUDIO_WAVEOUT)
  #include <ruby/audio/waveout.cpp>
#endif

#if defined(AUDIO_XAUDIO2)
  #include <ruby/audio/xaudio2.cpp>
#endif

#if defined(AUDIO_SDL)
#include <ruby/audio/sdl.cpp>
#endif

namespace ruby {

auto Audio::setExclusive(bool exclusive) -> bool {
  if(instance->exclusive == exclusive) return true;
  if(!instance->hasExclusive()) return false;
  if(!instance->setExclusive(instance->exclusive = exclusive)) return false;
  return true;
}

auto Audio::setContext(uintptr context) -> bool {
  if(instance->context == context) return true;
  if(!instance->hasContext()) return false;
  if(!instance->setContext(instance->context = context)) return false;
  return true;
}

auto Audio::setDevice(string device) -> bool {
  if(instance->device == device) return true;
  if(!instance->hasDevice(device)) return false;
  if(!instance->setDevice(instance->device = device)) return false;
  return true;
}

auto Audio::setBlocking(bool blocking) -> bool {
  if(instance->blocking == blocking) return true;
  if(!instance->hasBlocking()) return false;
  if(!instance->setBlocking(instance->blocking = blocking)) return false;
  updateResampleFrequency(instance->frequency);
  return true;
}

auto Audio::setDynamic(bool dynamic) -> bool {
  if(instance->dynamic == dynamic) return true;
  if(!instance->hasDynamic()) return false;
  if(!instance->setDynamic(instance->dynamic = dynamic)) return false;
  return true;
}

auto Audio::setChannels(u32 channels) -> bool {
  updateResampleChannels(channels);
  if(instance->channels == channels) return true;
  if(!instance->hasChannels(channels)) return false;
  if(!instance->setChannels(instance->channels = channels)) return false;
  return true;
}

auto Audio::setFrequency(u32 frequency) -> bool {
  if(instance->frequency == frequency) return true;
  if(!instance->hasFrequency(frequency)) return false;
  if(!instance->setFrequency(instance->frequency = frequency)) return false;
  updateResampleFrequency(instance->frequency);
  return true;
}

auto Audio::setLatency(u32 latency) -> bool {
  if(instance->latency == latency) return true;
  if(!instance->hasLatency(latency)) return false;
  if(!instance->setLatency(instance->latency = latency)) return false;
  return true;
}

//

auto Audio::updateResampleChannels(u32 channels) -> void {
  if(resamplers.size() != channels) {
    resamplers.clear();
    resamplers.resize(channels);
    updateResampleFrequency(instance->frequency);
    resampleBuffer.resize(channels);
  }
}

auto Audio::updateResampleFrequency(u32 frequency) -> void {
  for(auto& resampler : resamplers) resampler.reset(frequency);
}

//

auto Audio::clear() -> void {
  updateResampleFrequency(instance->frequency);
  return instance->clear();
}

auto Audio::level() -> f64 {
  return instance->level();
}

auto Audio::output(const f64 samples[]) -> void {
  if(!instance->dynamic) return instance->output(samples);

  f64 maxDelta = 0.005;
  f64 fillLevel = instance->level();
  f64 dynamicFrequency = ((1.0 - maxDelta) + 2.0 * fillLevel * maxDelta) * instance->frequency;
  for(auto& resampler : resamplers) {
    resampler.setInputFrequency(dynamicFrequency);
    resampler.write(*samples++);
  }

  while(!resamplers.empty() && resamplers.front().pending()) {
    for(u32 n : range(instance->channels)) resampleBuffer[n] = resamplers[n].read();
    instance->output(resampleBuffer.data());
  }
}

//

auto Audio::create(string driver) -> bool {
  self.instance.reset();
  if(!driver) driver = optimalDriver();

  #if defined(AUDIO_ALSA)
  if(driver == "ALSA") self.instance = std::make_unique<AudioALSA>(*this);
  #endif

  #if defined(AUDIO_AO)
  if(driver == "libao") self.instance = std::make_unique<AudioAO>(*this);
  #endif

  #if defined(AUDIO_ASIO)
  if(driver == "ASIO") self.instance = std::make_unique<AudioASIO>(*this);
  #endif

  #if defined(AUDIO_DIRECTSOUND)
  if(driver == "DirectSound 7.0") self.instance = std::make_unique<AudioDirectSound>(*this);
  #endif

  #if defined(AUDIO_OPENAL)
  if(driver == "OpenAL") self.instance = std::make_unique<AudioOpenAL>(*this);
  #endif

  #if defined(AUDIO_OSS)
  if(driver == "OSS") self.instance = std::make_unique<AudioOSS>(*this);
  #endif

  #if defined(AUDIO_PULSEAUDIO)
  if(driver == "PulseAudio") self.instance = std::make_unique<AudioPulseAudio>(*this);
  #endif

  #if defined(AUDIO_PULSEAUDIOSIMPLE)
  if(driver == "PulseAudio Simple") self.instance = std::make_unique<AudioPulseAudioSimple>(*this);
  #endif

  #if defined(AUDIO_WASAPI)
  if(driver == "WASAPI") self.instance = std::make_unique<AudioWASAPI>(*this);
  #endif

  #if defined(AUDIO_WAVEOUT)
  if(driver == "waveOut") self.instance = std::make_unique<AudioWaveOut>(*this);
  #endif

  #if defined(AUDIO_XAUDIO2)
  if(driver == "XAudio 2.9") self.instance = std::make_unique<AudioXAudio2>(*this);
  #endif

#if defined(AUDIO_SDL)
  if(driver == "SDL") self.instance = std::make_unique<AudioSDL>(*this);
#endif

  if(!self.instance) self.instance = std::make_unique<AudioDriver>(*this);

  return self.instance->create();
}

auto Audio::hasDrivers() -> std::vector<string> {
  return {

  #if defined(AUDIO_ASIO)
  "ASIO",
  #endif

  #if defined(AUDIO_WASAPI)
  "WASAPI",
  #endif

  #if defined(AUDIO_XAUDIO2)
  "XAudio 2.9",
  #endif

#if defined(AUDIO_SDL)
  "SDL",
#endif

  #if defined(AUDIO_DIRECTSOUND)
  "DirectSound 7.0",
  #endif

  #if defined(AUDIO_WAVEOUT)
  "waveOut",
  #endif

  #if defined(AUDIO_ALSA)
  "ALSA",
  #endif

  #if defined(AUDIO_OSS)
  "OSS",
  #endif

  #if defined(AUDIO_OPENAL)
  "OpenAL",
  #endif

  #if defined(AUDIO_PULSEAUDIO)
  "PulseAudio",
  #endif

  #if defined(AUDIO_PULSEAUDIOSIMPLE)
  "PulseAudio Simple",
  #endif

  #if defined(AUDIO_AO)
  "libao",
  #endif

  "None"};
}

auto Audio::optimalDriver() -> string {
  #if defined(AUDIO_WASAPI)
  return "WASAPI";
  #elif defined(AUDIO_ASIO)
  return "ASIO";
  #elif defined(AUDIO_XAUDIO2)
  return "XAudio 2.9";
  #elif defined(AUDIO_SDL)
  return "SDL";
  #elif defined(AUDIO_DIRECTSOUND)
  return "DirectSound 7.0";
  #elif defined(AUDIO_WAVEOUT)
  return "waveOut";
  #elif defined(AUDIO_PULSEAUDIO)
  return "PulseAudio";
  #elif defined(AUDIO_PULSEAUDIOSIMPLE)
  return "PulseAudio Simple";
  #elif defined(AUDIO_OPENAL)
  return "OpenAL";
  #elif defined(AUDIO_AO)
  return "libao";
  #elif defined(AUDIO_ALSA)
  return "ALSA";
  #elif defined(AUDIO_OSS)
  return "OSS";
  #else
  return "None";
  #endif
}

auto Audio::safestDriver() -> string {
  #if defined(AUDIO_WAVEOUT)
  return "waveOut";
  #elif defined(AUDIO_DIRECTSOUND)
  return "DirectSound 7.0";
  #elif defined(AUDIO_WASAPI)
  return "WASAPI";
  #elif defined(AUDIO_XAUDIO2)
  return "XAudio 2.9";
  #elif defined(AUDIO_SDL)
  return "SDL";
  #elif defined(AUDIO_ALSA)
  return "ALSA";
  #elif defined(AUDIO_OSS)
  return "OSS";
  #elif defined(AUDIO_OPENAL)
  return "OpenAL";
  #elif defined(AUDIO_PULSEAUDIO)
  return "PulseAudio";
  #elif defined(AUDIO_PULSEAUDIOSIMPLE)
  return "PulseAudio Simple";
  #elif defined(AUDIO_AO)
  return "libao";
  #elif defined(AUDIO_ASIO)
  return "ASIO";
  #else
  return "None";
  #endif
}

}
