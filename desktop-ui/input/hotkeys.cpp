auto InputManager::createHotkeys() -> void {
  static bool fastForwardVideoBlocking;
  static bool fastForwardAudioBlocking;
  static bool fastForwardAudioDynamic;
  static bool toggleFastForwardState = false;

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
    if(!toggleFastForwardState) {
      program.fastForwarding = true;
      fastForwardVideoBlocking = ruby::video.blocking();
      fastForwardAudioBlocking = ruby::audio.blocking();
      fastForwardAudioDynamic  = ruby::audio.dynamic();
      ruby::video.setBlocking(false);
      ruby::audio.setBlocking(false);
      ruby::audio.setDynamic(false);
    }
  }).onRelease([&] {
    Program::Guard guard;
    if(!emulator) return;
    if(!toggleFastForwardState) {
      program.fastForwarding = false;
      ruby::video.setBlocking(fastForwardVideoBlocking);
      ruby::audio.setBlocking(fastForwardAudioBlocking);
      ruby::audio.setDynamic(fastForwardAudioDynamic);
    }
  }));

  hotkeys.push_back(InputHotkey("Toggle Fast Forward").onPress([&] {
    Program::Guard guard;
    if(!emulator || program.rewinding) return;
    program.fastForwarding = !program.fastForwarding;

    if (program.fastForwarding) {
      toggleFastForwardState = true;
      fastForwardVideoBlocking = ruby::video.blocking();
      fastForwardAudioBlocking = ruby::audio.blocking();
      fastForwardAudioDynamic  = ruby::audio.dynamic();
      ruby::video.setBlocking(false);
      ruby::audio.setBlocking(false);
      ruby::audio.setDynamic(false);
      return;
    } 

    toggleFastForwardState = false;
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

  hotkeys.push_back(InputHotkey("Toggle Shader Display").onPress([&] {
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
  if(
    program.settingsWindowConstructed && settingsWindow.focused() ||
    program.toolsWindowConstructed && toolsWindow.focused()
  ) return;
  if(settings.input.defocus != "Allow") {
    if (!presentation.focused() && !ruby::video.fullScreen()) return;
  }

  struct PhysicalKey {
    u64 deviceID = 0;
    u32 groupID = 0;
    u32 inputID = 0;

    auto operator==(const PhysicalKey& other) const -> bool {
      return deviceID == other.deviceID && groupID == other.groupID && inputID == other.inputID;
    }
  };

  struct PhysicalKeyHash {
    auto operator()(const PhysicalKey& key) const -> size_t {
      auto hash = (size_t)key.deviceID;
      hash ^= (size_t)key.groupID + 0x9e3779b9 + (hash << 6) + (hash >> 2);
      hash ^= (size_t)key.inputID + 0x9e3779b9 + (hash << 6) + (hash >> 2);
      return hash;
    }
  };

  struct KeyBinding {
    std::shared_ptr<HID::Device> device;
    u64 deviceID = 0;
    u32 groupID = 0;
    u32 inputID = 0;
    InputMapping::Qualifier qualifier = InputMapping::Qualifier::None;
  };

  struct KeyEdge {
    bool down = false;
    bool pressed = false;
    bool released = false;
  };

  static std::unordered_map<PhysicalKey, bool, PhysicalKeyHash> previousDown;
  static std::vector<bool> previousBindingActive;

  auto stateIndex = [&](u32 hotkeyIndex, u32 bindingIndex) -> u32 {
    return hotkeyIndex * BindingLimit + bindingIndex;
  };

  auto parseAssignment = [&](string_view assignment) -> std::vector<KeyBinding> {
    std::vector<KeyBinding> result;
    if(assignment.size() == 0) return result;

    auto parts = nall::split(assignment, "+");
    result.reserve(parts.size());
    for(auto& part : parts) {
      auto token = nall::split(part.strip(), "/");
      if(token.size() < 3) {
        result.clear();
        return result;
      }

      KeyBinding key;
      key.deviceID = token[0].natural();
      key.groupID = token[1].natural();
      key.inputID = token[2].natural();
      if(result.empty() && token.size() > 3) {
        if(token[3] == "Lo") key.qualifier = InputMapping::Qualifier::Lo;
        if(token[3] == "Hi") key.qualifier = InputMapping::Qualifier::Hi;
        if(token[3] == "Rumble") key.qualifier = InputMapping::Qualifier::Rumble;
      }

      for(auto& device : devices) {
        if(device->id() == key.deviceID) {
          key.device = device;
          break;
        }
      }
      result.push_back(key);
    }

    return result;
  };

  auto readDown = [&](const KeyBinding& key) -> bool {
    if(!key.device) return false;
    if(key.groupID >= key.device->size()) return false;
    if(key.inputID >= key.device->group(key.groupID).size()) return false;

    s16 value = key.device->group(key.groupID).input(key.inputID).value();
    if(key.device->isKeyboard() && key.groupID == HID::Keyboard::GroupID::Button) return value != 0;
    if(key.device->isMouse() && key.groupID == HID::Mouse::GroupID::Button && ruby::input.acquired()) return value != 0;
    if(key.device->isJoypad() && key.groupID == HID::Joypad::GroupID::Button) return value != 0;
    if(key.device->isJoypad() && key.groupID != HID::Joypad::GroupID::Button) {
      if(key.qualifier == InputMapping::Qualifier::Lo) return value < -16384;
      if(key.qualifier == InputMapping::Qualifier::Hi) return value > +16384;
    }
    return false;
  };

  auto isKeyboardModifier = [&](const KeyBinding& key) -> bool {
    if(!key.device || !key.device->isKeyboard()) return false;
    if(key.groupID != HID::Keyboard::GroupID::Button) return false;
    if(key.groupID >= key.device->size()) return false;
    if(key.inputID >= key.device->group(key.groupID).size()) return false;

    auto name = key.device->group(key.groupID).input(key.inputID).name();
    return name == "Shift" || name == "LeftShift" || name == "RightShift"
        || name == "Control" || name == "LeftControl" || name == "RightControl"
        || name == "Option" || name == "LeftAlt" || name == "RightAlt"
        || name == "Command" || name == "Super" || name == "LeftSuper" || name == "RightSuper";
  };

  const u32 bindingCount = hotkeys.size() * BindingLimit;
  if(previousBindingActive.size() != bindingCount) {
    previousBindingActive.assign(bindingCount, false);
  }

  std::vector<std::vector<KeyBinding>> parsedBindings(bindingCount);
  std::unordered_map<PhysicalKey, KeyBinding, PhysicalKeyHash> referencedKeys;
  for(u32 hotkeyIndex : range(hotkeys.size())) {
    auto& hotkey = hotkeys[hotkeyIndex];
    for(u32 bindingIndex : range(BindingLimit)) {
      auto index = stateIndex(hotkeyIndex, bindingIndex);
      auto parsed = parseAssignment(hotkey.assignments[bindingIndex]);
      if(parsed.empty()) continue;
      parsedBindings[index] = std::move(parsed);

      for(auto& key : parsedBindings[index]) {
        auto physical = PhysicalKey{key.deviceID, key.groupID, key.inputID};
        if(referencedKeys.find(physical) == referencedKeys.end()) referencedKeys.emplace(physical, key);
      }
    }
  }

  std::unordered_map<PhysicalKey, KeyEdge, PhysicalKeyHash> keyEdges;
  keyEdges.reserve(referencedKeys.size());
  for(auto& [physical, key] : referencedKeys) {
    auto down = readDown(key);
    auto previous = previousDown.find(physical) != previousDown.end() ? previousDown[physical] : false;
    keyEdges[physical] = {down, !previous && down, previous && !down};
  }

  auto edgeOf = [&](const KeyBinding& key) -> KeyEdge {
    auto found = keyEdges.find(PhysicalKey{key.deviceID, key.groupID, key.inputID});
    if(found == keyEdges.end()) return {};
    return found->second;
  };

  auto keyboardModifiersMatchExactly = [&](const std::vector<KeyBinding>& keys) -> bool {
    std::vector<KeyBinding> keyboardKeys;
    keyboardKeys.reserve(keys.size());
    for(auto& key : keys) {
      if(key.device && key.device->isKeyboard() && key.groupID == HID::Keyboard::GroupID::Button) {
        keyboardKeys.push_back(key);
      }
    }
    if(keyboardKeys.empty()) return true;

    std::unordered_map<u64, std::vector<PhysicalKey>> expectedModifiers;
    for(auto& key : keyboardKeys) {
      if(isKeyboardModifier(key)) {
        expectedModifiers[key.deviceID].push_back({key.deviceID, key.groupID, key.inputID});
      } else if(expectedModifiers.find(key.deviceID) == expectedModifiers.end()) {
        expectedModifiers[key.deviceID] = {};
      }
    }

    for(auto& [deviceID, modifiers] : expectedModifiers) {
      std::shared_ptr<HID::Device> device;
      for(auto& key : keyboardKeys) {
        if(key.deviceID == deviceID) {
          device = key.device;
          break;
        }
      }
      if(!device) return false;

      for(u32 inputID : range(device->group(HID::Keyboard::GroupID::Button).size())) {
        KeyBinding candidate;
        candidate.device = device;
        candidate.deviceID = deviceID;
        candidate.groupID = HID::Keyboard::GroupID::Button;
        candidate.inputID = inputID;
        if(!isKeyboardModifier(candidate) || !edgeOf(candidate).down) continue;

        bool expected = false;
        for(auto& modifier : modifiers) {
          if(modifier.inputID == inputID) {
            expected = true;
            break;
          }
        }
        if(!expected) return false;
      }
    }

    return true;
  };

  std::vector<bool> bindingDown(bindingCount, false);
  std::vector<bool> bindingActivated(bindingCount, false);
  std::unordered_map<PhysicalKey, bool, PhysicalKeyHash> claimedSuffixPress;

  for(u32 hotkeyIndex : range(hotkeys.size())) {
    for(u32 bindingIndex : range(BindingLimit)) {
      auto index = stateIndex(hotkeyIndex, bindingIndex);
      auto& keys = parsedBindings[index];
      if(keys.empty()) {
        previousBindingActive[index] = false;
        continue;
      }

      bool down = true;
      for(auto& key : keys) {
        if(!edgeOf(key).down) {
          down = false;
          break;
        }
      }
      if(down && !keyboardModifiersMatchExactly(keys)) down = false;
      bindingDown[index] = down;

      if(!previousBindingActive[index] && down) {
        if(keys.size() == 1) {
          bindingActivated[index] = true;
        } else if(edgeOf(keys[keys.size() - 1]).pressed) {
          bindingActivated[index] = true;
          auto key = keys[keys.size() - 1];
          claimedSuffixPress[PhysicalKey{key.deviceID, key.groupID, key.inputID}] = true;
        }
      }
      previousBindingActive[index] = down;
    }
  }

  for(u32 hotkeyIndex : range(hotkeys.size())) {
    auto& hotkey = hotkeys[hotkeyIndex];
    bool wasActive = hotkey.state != 0;
    bool isActive = false;
    bool activated = false;

    for(u32 bindingIndex : range(BindingLimit)) {
      auto index = stateIndex(hotkeyIndex, bindingIndex);
      auto& keys = parsedBindings[index];
      if(keys.empty()) continue;
      isActive |= bindingDown[index];

      if(bindingActivated[index]) {
        if(keys.size() == 1) {
          auto key = keys[0];
          if(claimedSuffixPress.find(PhysicalKey{key.deviceID, key.groupID, key.inputID}) != claimedSuffixPress.end()) {
            continue;
          }
        }
        activated = true;
      }
    }

    if(!wasActive && activated && hotkey.press) hotkey.press();
    if(wasActive && !isActive && hotkey.release) hotkey.release();
    hotkey.state = isActive ? 1 : 0;
  }

  previousDown.clear();
  for(auto& [physical, edge] : keyEdges) {
    previousDown[physical] = edge.down;
  }
}
