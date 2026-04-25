auto Program::videoDriverUpdate() -> void {
  Program::Guard guard;
  ruby::video.create(settings.video.driver);
  ruby::video.setContext(presentation.viewport.handle());
  videoMonitorUpdate();
  videoFormatUpdate();
  ruby::video.setExclusive(settings.video.exclusive);
  ruby::video.setBlocking(settings.video.blocking);
  ruby::video.setFlush(settings.video.flush);
  ruby::video.setShader(settings.video.shader);
  ruby::video.setForceSRGB(settings.video.forceSRGB);
  ruby::video.setThreadedRenderer(settings.video.threadedRenderer);
  ruby::video.setNativeFullScreen(settings.video.nativeFullScreen);

  if(!ruby::video.ready()) {
    driverInitFailed(settings.video.driver, "video", [&] { driverSettings.videoDriverUpdate(); });
    return;
  }

  if(startShader && !Presentation::shaderArgApplied) {
    Presentation::shaderArgApplied = true;
    string location = locate("Shaders/");
    #if defined(PLATFORM_LINUX) || defined(PLATFORM_BSD)
    if(!inode::exists(location)) location = locate("../libretro/shaders/shaders_slang/");
    #endif

    string existingShader = settings.video.shader;
    startShader.transform("\\", "/");
    if(!startShader.imatch("None")) {
      settings.video.shader = {startShader, ".slangp"};
    } else {
      settings.video.shader = startShader;
    }

    if(inode::exists({location, settings.video.shader})) {
      ruby::video.setShader({location, settings.video.shader});
    } else if(settings.video.shader.imatch("None")) {
      ruby::video.setShader("None");
    } else {
      if(kiosk) {
        showMessage({"Requested shader not found: ", location, settings.video.shader, ". Using existing shader."});
      } else {
        hiro::MessageDialog()
          .setTitle("Warning")
          .setAlignment(hiro::Alignment::Center)
          .setText({"Requested shader not found: ", location, settings.video.shader,
            "\nUsing existing defined shader: ", location, existingShader})
          .warning();
      }
      settings.video.shader = existingShader;
    }
  }

  if(!kiosk) presentation.loadShaders();
}

auto Program::videoMonitorUpdate() -> void {
  Program::Guard guard;
  if(!ruby::video.hasMonitor(settings.video.monitor)) {
    settings.video.monitor = ruby::video.monitor();
  }
  ruby::video.setMonitor(settings.video.monitor);
}

auto Program::videoFormatUpdate() -> void {
  Program::Guard guard;
  if(!ruby::video.hasFormat(settings.video.format)) {
    settings.video.format = ruby::video.format();
  }
  ruby::video.setFormat(settings.video.format);
}

auto Program::videoFullScreenToggle() -> void {
  Program::Guard guard;
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

auto Program::videoPseudoFullScreenToggle() -> void {
  Program::Guard guard;
  if(ruby::video.fullScreen()) return;

  ruby::video.clear();
  if(!presentation.fullScreen()) {
    presentation.setFullScreen(true);
    presentation.menuBar.setVisible(false);
    if(!ruby::input.acquired() && ruby::video.hasMonitors().size() == 1) {
      ruby::input.acquire();
    }
    startPseudoFullScreen = true;
  } else {
    if(ruby::input.acquired()) {
      ruby::input.release();
    }
    if(!kiosk) presentation.menuBar.setVisible(true);
    presentation.setFullScreen(false);
    presentation.viewport.setFocused();
    startPseudoFullScreen = false;
  }
}

auto Program::audioDriverUpdate() -> void {
  Program::Guard guard;
  ruby::audio.create(settings.audio.driver);
  ruby::audio.setContext(presentation.viewport.handle());
  audioDeviceUpdate();
  audioFrequencyUpdate();
  audioLatencyUpdate();
  ruby::audio.setExclusive(settings.audio.exclusive);
  ruby::audio.setBlocking(settings.audio.blocking);
  ruby::audio.setDynamic(settings.audio.dynamic);

  if(!ruby::audio.ready()) {
    driverInitFailed(settings.audio.driver, "audio", [&] { driverSettings.audioDriverUpdate(); });
    return;
  }
}

auto Program::audioDeviceUpdate() -> void {
  Program::Guard guard;
  if(!ruby::audio.hasDevice(settings.audio.device)) {
    settings.audio.device = ruby::audio.device();
  }
  ruby::audio.setDevice(settings.audio.device);
}

auto Program::audioFrequencyUpdate() -> void {
  Program::Guard guard;
  if(!ruby::audio.hasFrequency(settings.audio.frequency)) {
    settings.audio.frequency = ruby::audio.frequency();
  }
  ruby::audio.setFrequency(settings.audio.frequency);

  for(auto& stream : streams) {
    stream->setResamplerFrequency(ruby::audio.frequency());
  }
}

auto Program::audioLatencyUpdate() -> void {
  Program::Guard guard;
  if(!ruby::audio.hasLatency(settings.audio.latency)) {
    settings.audio.latency = ruby::audio.latency();
  }
  ruby::audio.setLatency(settings.audio.latency);
}

auto Program::inputDriverUpdate() -> void {
  Program::Guard guard;
  ruby::input.create(settings.input.driver);
  ruby::input.setContext(presentation.viewport.handle());
  ruby::input.onChange(std::bind_front(&InputManager::eventInput, &inputManager));

  if(!ruby::input.ready()) {
    driverInitFailed(settings.input.driver, "input", [&] { driverSettings.inputDriverUpdate(); });
    return;
  }

  inputManager.poll(true);
}

auto Program::driverInitFailed(nall::string& driver, const char* kind, auto&& updateSettingsWindow) -> void {
  error({"Failed to initialize ", driver, " ", kind, " driver."});

  driver = "None";
  if(settingsWindowConstructed) updateSettingsWindow();
}
