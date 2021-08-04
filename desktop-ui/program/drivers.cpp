auto Program::videoDriverUpdate() -> void {
  ruby::video.create(settings.video.driver);
  ruby::video.setContext(presentation.viewport.handle());
  videoMonitorUpdate();
  videoFormatUpdate();
  ruby::video.setExclusive(settings.video.exclusive);
  ruby::video.setBlocking(settings.video.blocking);
  ruby::video.setFlush(settings.video.flush);
  ruby::video.setShader(settings.video.shader);

  if(!ruby::video.ready()) {
    MessageDialog().setText({"Failed to initialize ", settings.video.driver, " video driver."}).setAlignment(presentation).error();
    settings.video.driver = "None";
    driverSettings.videoDriverUpdate();
  }

  presentation.loadShaders();
}

auto Program::videoMonitorUpdate() -> void {
  if(!ruby::video.hasMonitor(settings.video.monitor)) {
    settings.video.monitor = ruby::video.monitor();
  }
  ruby::video.setMonitor(settings.video.monitor);
}

auto Program::videoFormatUpdate() -> void {
  if(!ruby::video.hasFormat(settings.video.format)) {
    settings.video.format = ruby::video.format();
  }
  ruby::video.setFormat(settings.video.format);
}

auto Program::videoFullScreenToggle() -> void {
  if(!ruby::video.hasFullScreen()) return;

  ruby::video.clear();
  if(!ruby::video.fullScreen()) {
    ruby::video.setFullScreen(true);
    if(!ruby::input.acquired()) {
      if(ruby::video.exclusive() || ruby::video.hasMonitors().size() == 1) {
        ruby::input.acquire();
      }
    }
  } else {
    if(ruby::input.acquired()) {
      ruby::input.release();
    }
    ruby::video.setFullScreen(false);
    presentation.viewport.setFocused();
  }
}

//

auto Program::audioDriverUpdate() -> void {
  ruby::audio.create(settings.audio.driver);
  ruby::audio.setContext(presentation.viewport.handle());
  audioDeviceUpdate();
  audioFrequencyUpdate();
  audioLatencyUpdate();
  ruby::audio.setExclusive(settings.audio.exclusive);
  ruby::audio.setBlocking(settings.audio.blocking);
  ruby::audio.setDynamic(settings.audio.dynamic);

  if(!ruby::audio.ready()) {
    MessageDialog().setText({"Failed to initialize ", settings.audio.driver, " audio driver."}).setAlignment(presentation).error();
    settings.audio.driver = "None";
    driverSettings.audioDriverUpdate();
  }
}

auto Program::audioDeviceUpdate() -> void {
  if(!ruby::audio.hasDevice(settings.audio.device)) {
    settings.audio.device = ruby::audio.device();
  }
  ruby::audio.setDevice(settings.audio.device);
}

auto Program::audioFrequencyUpdate() -> void {
  if(!ruby::audio.hasFrequency(settings.audio.frequency)) {
    settings.audio.frequency = ruby::audio.frequency();
  }
  ruby::audio.setFrequency(settings.audio.frequency);

  for(auto& stream : streams) {
    stream->setResamplerFrequency(ruby::audio.frequency());
  }
}

auto Program::audioLatencyUpdate() -> void {
  if(!ruby::audio.hasLatency(settings.audio.latency)) {
    settings.audio.latency = ruby::audio.latency();
  }
  ruby::audio.setLatency(settings.audio.latency);
}

//

auto Program::inputDriverUpdate() -> void {
  ruby::input.create(settings.input.driver);
  ruby::input.setContext(presentation.viewport.handle());
  ruby::input.onChange({&InputManager::eventInput, &inputManager});

  if(!ruby::input.ready()) {
    MessageDialog().setText({"Failed to initialize ", settings.input.driver, " input driver."}).setAlignment(presentation).error();
    settings.input.driver = "None";
    driverSettings.inputDriverUpdate();
  }

  inputManager.poll(true);
}
