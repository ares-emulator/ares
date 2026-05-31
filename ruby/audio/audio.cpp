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

auto Audio::create(string driver) -> bool {
  self.instance.reset();
  if(!driver) driver = optimalDriver();

  #if defined(AUDIO_SDL)
  if(driver == "SDL") self.instance = std::make_unique<AudioSDL>(*this);
  #endif

  if(!self.instance) self.instance = std::make_unique<AudioDriver>(*this);

  return self.instance->create();
}

auto Audio::hasDrivers() -> std::vector<string> {
  return {

  #if defined(AUDIO_SDL)
  "SDL",
  #endif

  "None"};
}

auto Audio::optimalDriver() -> string {
  #if defined(AUDIO_SDL)
  return "SDL";
  #else
  return "None";
  #endif
}

auto Audio::safestDriver() -> string {
  #if defined(AUDIO_SDL)
  return "SDL";
  #else
  return "None";
  #endif
}

}
