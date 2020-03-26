InputHotkey::InputHotkey(string_view name) : name(name) {
}

auto InputHotkey::poll() -> void {
  //don't allow hotkeys to trigger while emulator is unfocused
  if(!videoInstance.fullScreen()) {
    if(!program.focused()) return;
  } else {
    if(!videoInstance.focused()) return;
  }

  //don't allow hotkeys to trigger while mapping them
  if(hotkeySettings.visible()) return;

  if(device && !hotkeySettings.visible()) {
    newValue = device->group(groupID).input(inputID).value();
    if(oldValue == 0 && newValue == 1 && onPress) onPress();
    if(oldValue == 1 && newValue == 0 && onRelease) onRelease();
    oldValue = newValue;
  }
}

Hotkeys::Hotkeys() {
  toggleStatus.onPress = [&] {
    if(program.statusLayout.visible()) {
      program.hideStatus();
    } else {
      program.showStatus();
    }
  };
  hotkeys.append(&toggleStatus);

  togglePanels.onPress = [&] {
    if(program.panelLayout.visible()) {
      program.hidePanels();
    } else {
      program.showPanels();
    }
  };
  hotkeys.append(&togglePanels);

  toggleFullscreen.onPress = [&] {
    if(!emulator.system.power) return;
    emulator.videoToggleFullscreen();
  };
  hotkeys.append(&toggleFullscreen);

  toggleMouseCapture.onPress = [&] {
    if(!emulator.system.power) return;
    if(!inputInstance.acquired()) {
      inputInstance.acquire();
    } else {
      inputInstance.release();
    }
  };
  hotkeys.append(&toggleMouseCapture);

  fastForward.onPress = [&] {
    if(!interface) return;
    fastForwardVideoBlocking = videoInstance.blocking();
    fastForwardAudioBlocking = audioInstance.blocking();
    fastForwardAudioDynamic = audioInstance.dynamic();
    videoInstance.setBlocking(false);
    audioInstance.setBlocking(false);
    audioInstance.setDynamic(false);
  };
  fastForward.onRelease = [&] {
    if(!interface) return;
    videoInstance.setBlocking(fastForwardVideoBlocking);
    audioInstance.setBlocking(fastForwardAudioBlocking);
    audioInstance.setDynamic(fastForwardAudioDynamic);
  };
  hotkeys.append(&fastForward);

  saveState.onPress = [&] {
    emulator.saveState(stateSlot);
  };
  hotkeys.append(&saveState);

  loadState.onPress = [&] {
    emulator.loadState(stateSlot);
  };
  hotkeys.append(&loadState);

  incrementStateSlot.onPress = [&] {
    if(stateSlot < 5) stateSlot++;
    else stateSlot = 1;
  };
  hotkeys.append(&incrementStateSlot);

  decrementStateSlot.onPress = [&] {
    if(stateSlot > 1) stateSlot--;
    else stateSlot = 5;
  };
  hotkeys.append(&decrementStateSlot);

  pauseEmulation.onPress = [&] {
    if(!interface) return;
    toolsMenu.pauseEmulation.setChecked(!toolsMenu.pauseEmulation.checked());
  };
  hotkeys.append(&pauseEmulation);

  exportMemory.onPress = [&] {
    if(!interface) return;
    interface->exportMemory();
  };
  hotkeys.append(&exportMemory);

  quitEmulator.onPress = [&] {
    emulator.quit();
  };
  hotkeys.append(&quitEmulator);
}

auto Hotkeys::poll() -> void {
  for(auto& hotkey : hotkeys) hotkey->poll();
}

auto Hotkeys::bind() -> void {
  for(auto& hotkey : hotkeys) {
    hotkey->device.reset();
    auto part = hotkey->identifier.split("/");
    if(part.size() != 5) continue;

    hotkey->pathID = part[0].hex();
    hotkey->vendorID = part[1].hex();
    hotkey->productID = part[2].hex();
    string groupName = part[3];
    string inputName = part[4];

    for(auto& device : inputManager.devices) {
      if(hotkey->pathID != device->pathID()) continue;
      if(hotkey->vendorID != device->vendorID()) continue;
      if(hotkey->productID != device->productID()) continue;
      if(auto groupID = device->find(groupName)) {
        if(auto inputID = device->group(groupID()).find(inputName)) {
          hotkey->device = device;
          hotkey->groupID = groupID();
          hotkey->inputID = inputID();
        }
      }
    }
  }
}
