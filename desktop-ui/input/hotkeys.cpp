auto InputManager::createHotkeys() -> void {
  static bool fastForwardVideoBlocking;
  static bool fastForwardAudioBlocking;
  static bool fastForwardAudioDynamic;

  hotkeys.append(InputHotkey("Toggle Fullscreen").onPress([&] {
    program.videoFullScreenToggle();
  }));

  hotkeys.append(InputHotkey("Toggle Mouse Capture").onPress([&] {
    if(!emulator) return;
    if(!ruby::input.acquired()) {
      ruby::input.acquire();
    } else {
      ruby::input.release();
    }
  }));

  hotkeys.append(InputHotkey("Toggle Keyboard Capture").onPress([&] {
    if(!emulator) return;
    program.keyboardCaptured = !program.keyboardCaptured;
    print("Keyboard capture: ", program.keyboardCaptured, "\n");
  }));

  hotkeys.append(InputHotkey("Fast Forward").onPress([&] {
    if(!emulator || program.rewinding) return;
    program.fastForwarding = true;
    fastForwardVideoBlocking = ruby::video.blocking();
    fastForwardAudioBlocking = ruby::audio.blocking();
    fastForwardAudioDynamic  = ruby::audio.dynamic();
    ruby::video.setBlocking(false);
    ruby::audio.setBlocking(false);
    ruby::audio.setDynamic(false);
  }).onRelease([&] {
    if(!emulator) return;
    program.fastForwarding = false;
    ruby::video.setBlocking(fastForwardVideoBlocking);
    ruby::audio.setBlocking(fastForwardAudioBlocking);
    ruby::audio.setDynamic(fastForwardAudioDynamic);
  }));

  hotkeys.append(InputHotkey("Toggle Fast Forward").onPress([&] {
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

  hotkeys.append(InputHotkey("Rewind").onPress([&] {
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

  hotkeys.append(InputHotkey("Frame Advance").onPress([&] {
    if(!emulator) return;
    if(!program.paused) program.pause(true);
    program.requestFrameAdvance = true;
  }));

  hotkeys.append(InputHotkey("Capture Screenshot").onPress([&] {
    if(!emulator) return;
    program.requestScreenshot = true;
  }));

  hotkeys.append(InputHotkey("Save State").onPress([&] {
    if(!emulator) return;
    program.stateSave(program.state.slot);
  }));

  hotkeys.append(InputHotkey("Load State").onPress([&] {
    if(!emulator) return;
    program.stateLoad(program.state.slot);
  }));

  hotkeys.append(InputHotkey("Decrement State Slot").onPress([&] {
    if(!emulator) return;
    if(program.state.slot == 1) program.state.slot = 9;
    else program.state.slot--;
    program.showMessage({"Selected state slot ", program.state.slot});
  }));

  hotkeys.append(InputHotkey("Increment State Slot").onPress([&] {
    if(!emulator) return;
    if(program.state.slot == 9) program.state.slot = 1;
    else program.state.slot++;
    program.showMessage({"Selected state slot ", program.state.slot});
  }));

  hotkeys.append(InputHotkey("Pause Emulation").onPress([&] {
    if(!emulator) return;
    program.pause(!program.paused);
  }));

  hotkeys.append(InputHotkey("Reset System").onPress([&] {
    if(!emulator) return;
    emulator->root->power(true);
  }));

  hotkeys.append(InputHotkey("Reload Current Game").onPress([&] {
    if(!emulator) return;
    program.load(emulator, emulator->game->location);
  }));

  hotkeys.append(InputHotkey("Quit Emulator").onPress([&] {
    program.quit();
  }));

  hotkeys.append(InputHotkey("Mute Audio").onPress([&] {
    if(!emulator) return;
    program.mute();
  }));

  hotkeys.append(InputHotkey("Increase Audio").onPress([&] {
    if(!emulator) return;
    if(settings.audio.volume <= (f64)(1.9)) settings.audio.volume += (f64)(0.1);
  }));

  hotkeys.append(InputHotkey("Decrease Audio").onPress([&] {
    if(!emulator) return;
    if(settings.audio.volume >= (f64)(0.1)) settings.audio.volume -= (f64)(0.1);
  }));

#if defined(PLATFORM_WINDOWS)
  hotkeys.append(InputHotkey("Toggle Borderless Window").onPress([&] {
    program.updateBorderless();
  }));
#endif
}

auto InputManager::pollHotkeys() -> void {
  if(Application::modal()) return;

  if(!driverSettings.inputDefocusAllow.checked()) {
    if (!presentation.focused() && !ruby::video.fullScreen()) return;
  }

  for(auto& hotkey : hotkeys) {
    auto state = hotkey.value();
    if(hotkey.state == 0 && state == 1 && hotkey.press) hotkey.press();
    if(hotkey.state == 1 && state == 0 && hotkey.release) hotkey.release();
    hotkey.state = state;
  }
}
