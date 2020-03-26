auto Emulator::audioUpdate() -> void {
  if(audioInstance && audioInstance.driver() != settings.audio.driver) {
    audioInstance.reset();
  }

  if(!audioInstance) {
    audioInstance.create(settings.audio.driver);
    audioInstance.setContext(program.handle());
    if(!audioInstance.ready()) {
      audioInstance.create(settings.audio.driver = "None");
    }
  }

  if(audioInstance.hasDevice(settings.audio.device)) {
    audioInstance.setDevice(settings.audio.device);
  } else {
    settings.audio.device = audioInstance.device();
  }

  if(audioInstance.hasFrequency(settings.audio.frequency)) {
    audioInstance.setFrequency(settings.audio.frequency);
  } else {
    settings.audio.frequency = audioInstance.frequency();
  }

  if(audioInstance.hasLatency(settings.audio.latency)) {
    audioInstance.setLatency(settings.audio.latency);
  } else {
    settings.audio.latency = audioInstance.latency();
  }

  if(audioInstance.hasExclusive()) {
    audioInstance.setExclusive(settings.audio.exclusive);
  } else {
    settings.audio.exclusive = false;
  }

  if(audioInstance.hasBlocking()) {
    audioInstance.setBlocking(settings.audio.blocking);
  } else {
    settings.audio.blocking = false;
  }

  if(audioInstance.hasDynamic()) {
    audioInstance.setDynamic(settings.audio.dynamic);
  } else {
    settings.audio.dynamic = false;
  }
}

auto Emulator::audioUpdateEffects() -> void {
  if(!interface) return;
  for(auto stream : root->find<ares::Node::Stream>()) {
    stream->setResamplerFrequency(settings.audio.frequency + settings.audio.skew);
  }
}
