struct InputHotkey {
  InputHotkey(string_view name);
  auto poll() -> void;

  const string name;
  string identifier;

  shared_pointer<HID::Device> device;
  uint pathID = 0;
  uint vendorID = 0;
  uint productID = 0;
  uint groupID = 0;
  uint inputID = 0;

  bool oldValue = 0;
  bool newValue = 0;

  function<void ()> onPress;
  function<void ()> onRelease;
};

struct Hotkeys {
  Hotkeys();
  auto poll() -> void;
  auto bind() -> void;

  InputHotkey toggleStatus{"Toggle Status Bar"};
  InputHotkey togglePanels{"Toggle System Panels"};
  InputHotkey toggleFullscreen{"Toggle Fullscreen"};
  InputHotkey toggleMouseCapture{"Toggle Mouse Capture"};
  InputHotkey fastForward{"Fast Forward"};
  InputHotkey saveState{"Save State"};
  InputHotkey loadState{"Load State"};
  InputHotkey incrementStateSlot{"Increment State Slot"};
  InputHotkey decrementStateSlot{"Decrement State Slot"};
  InputHotkey pauseEmulation{"Pause Emulation"};
  InputHotkey exportMemory{"Export Memory"};
  InputHotkey quitEmulator{"Quit Emulator"};

  vector<InputHotkey*> hotkeys;

private:
  uint stateSlot = 1;
  bool fastForwardVideoBlocking = false;
  bool fastForwardAudioBlocking = false;
  bool fastForwardAudioDynamic = false;
};
