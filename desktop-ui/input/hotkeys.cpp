auto InputManager::createHotkeys() -> void {
  static bool fastForwardVideoBlocking;
  static bool fastForwardAudioBlocking;
  static bool fastForwardAudioDynamic;

  hotkeys.push_back(InputHotkey("Toggle Fullscreen").onPress([&] {
    program.videoFullScreenToggle();
  }));

  hotkeys.push_back(InputHotkey("Toggle Pseudo-Fullscreen").onPress([&] {
    program.videoPseudoFullScreenToggle();
  }));

  hotkeys.push_back(InputHotkey("Toggle Mouse Capture").onPress([&] {
    Program::Guard guard;
    if(!emulator) return;
    if(!ruby::input.acquired()) {
      ruby::input.acquire();
    } else {
      ruby::input.release();
    }
  }));

  hotkeys.push_back(InputHotkey("Toggle Keyboard Capture").onPress([&] {
    Program::Guard guard;
    if(!emulator) return;
    program.keyboardCaptured = !program.keyboardCaptured;
    print("Keyboard capture: ", program.keyboardCaptured, "\n");
  }));

  hotkeys.push_back(InputHotkey("Fast Forward").onPress([&] {
    Program::Guard guard;
    if(!emulator || program.rewinding) return;
    program.fastForwarding = true;
    fastForwardVideoBlocking = ruby::video.blocking();
    fastForwardAudioBlocking = ruby::audio.blocking();
    fastForwardAudioDynamic  = ruby::audio.dynamic();
    ruby::video.setBlocking(false);
    ruby::audio.setBlocking(false);
    ruby::audio.setDynamic(false);
  }).onRelease([&] {
    Program::Guard guard;
    if(!emulator) return;
    program.fastForwarding = false;
    ruby::video.setBlocking(fastForwardVideoBlocking);
    ruby::audio.setBlocking(fastForwardAudioBlocking);
    ruby::audio.setDynamic(fastForwardAudioDynamic);
  }));

  hotkeys.push_back(InputHotkey("Toggle Fast Forward").onPress([&] {
    Program::Guard guard;
    if(!emulator || program.rewinding) return;
    program.fastForwarding = !program.fastForwarding;

    if (program.fastForwarding) {
      fastForwardVideoBlocking = ruby::video.blocking();
      fastForwardAudioBlocking = ruby::audio.blocking();
      fastForwardAudioDynamic  = ruby::audio.dynamic();
      ruby::video.setBlocking(false);
      ruby::audio.setBlocking(false);
      ruby::audio.setDynamic(false);
      return;
    } 

    ruby::video.setBlocking(fastForwardVideoBlocking);
    ruby::audio.setBlocking(fastForwardAudioBlocking);
    ruby::audio.setDynamic(fastForwardAudioDynamic);
  }));

  hotkeys.push_back(InputHotkey("Rewind").onPress([&] {
    Program::Guard guard;
    if(!emulator || program.fastForwarding) return;
    if(program.rewind.frequency == 0) {
      return program.showMessage("Please enable rewind support in the emulator settings first.");
    }
    program.rewinding = true;
    program.rewindSetMode(Program::Rewind::Mode::Rewinding);
  }).onRelease([&] {
    if(!emulator) return;
    program.rewinding = false;
    program.rewindSetMode(Program::Rewind::Mode::Playing);
  }));

  hotkeys.push_back(InputHotkey("Frame Advance").onPress([&] {
    Program::Guard guard;
    if(!emulator) return;
    if(!program.paused) program.pause(true);
    program.requestFrameAdvance = true;
  }));

  hotkeys.push_back(InputHotkey("Capture Screenshot").onPress([&] {
    Program::Guard guard;
    if(!emulator) return;
    program.requestScreenshot = true;
  }));

  hotkeys.push_back(InputHotkey("Save State").onPress([&] {
    Program::Guard guard;
    if(!emulator) return;
    program.stateSave(program.state.slot);
  }));

  hotkeys.push_back(InputHotkey("Load State").onPress([&] {
    Program::Guard guard;
    if(!emulator) return;
    program.stateLoad(program.state.slot);
  }));

  hotkeys.push_back(InputHotkey("Decrement State Slot").onPress([&] {
    if(!emulator) return;
    if(program.state.slot == 1) program.state.slot = 9;
    else program.state.slot--;
    program.showMessage({"Selected state slot ", program.state.slot});
  }));

  hotkeys.push_back(InputHotkey("Increment State Slot").onPress([&] {
    if(!emulator) return;
    if(program.state.slot == 9) program.state.slot = 1;
    else program.state.slot++;
    program.showMessage({"Selected state slot ", program.state.slot});
  }));

  hotkeys.push_back(InputHotkey("Pause Emulation").onPress([&] {
    Program::Guard guard;
    if(!emulator) return;
    program.pause(!program.paused);
  }));

  hotkeys.push_back(InputHotkey("Reset System").onPress([&] {
    Program::Guard guard;
    if(!emulator) return;
    emulator->root->power(true);
  }));

  hotkeys.push_back(InputHotkey("Reload Current Game").onPress([&] {
    Program::Guard guard;
    if(!emulator) return;
    program.load(emulator, emulator->game->location);
  }));

  hotkeys.push_back(InputHotkey("Quit Emulator").onPress([&] {
    Program::Guard guard;
    program.quit();
  }));

  hotkeys.push_back(InputHotkey("Mute Audio").onPress([&] {
    if(!emulator) return;
    program.mute();
  }));

  hotkeys.push_back(InputHotkey("Increase Audio").onPress([&] {
    if(!emulator) return;
    if(settings.audio.volume <= (f64)(1.9)) settings.audio.volume += (f64)(0.1);
  }));

  hotkeys.push_back(InputHotkey("Decrease Audio").onPress([&] {
    if(!emulator) return;
    if(settings.audio.volume >= (f64)(0.1)) settings.audio.volume -= (f64)(0.1);
  }));

  hotkeys.push_back(InputHotkey("Enable/Disable Shader").onPress([&] {
    if(!emulator) return;
    if(!settings.video.shader.imatch("None")) {
      if(ruby::video.shader().imatch("None")) {
        ruby::video.setShader({locate("Shaders/"), settings.video.shader});
      } else {
        ruby::video.setShader("None");
      }
    }
  }));
}

auto InputManager::pollHotkeys() -> void {
  if(Application::modal()) return;

  if(settings.input.defocus != "Allow") {
    if (!presentation.focused() && !ruby::video.fullScreen()) return;
  }

  for(auto& hotkey : hotkeys) {
    auto state = hotkey.value();
    if(hotkey.state == 0 && state == 1 && hotkey.press) hotkey.press();
    if(hotkey.state == 1 && state == 0 && hotkey.release) hotkey.release();
    hotkey.state = state;
  }
}
