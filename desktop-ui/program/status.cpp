auto Program::updateMessage() -> void {
  // This function is called every iteration of the GUI run loop. Acquiring the emulator mutex would incur a severe
  // responsiveness penalty, so use a dedicated mutex for message passing.
  lock_guard<recursive_mutex> messageLock(_messageMutex);
  if(chrono::millisecond() - message.timestamp >= 2000) {
    message = {};
    if(!messages.empty()) { message = messages.front(); messages.erase(messages.begin()); }
  }

  if(message.text.length() > 0) {
    presentation.statusLeft.setText(message.text);
  } else if(settings.debugServer.enabled) {
    presentation.statusLeft.setText(nall::GDB::server.getStatusText(settings.debugServer.port, settings.debugServer.useIPv4));
  } else if(configuration) {
    presentation.statusLeft.setText(configuration);
  } else {
    presentation.statusLeft.setText();
  }

  static u64 shownAt = 0;
  static s64 shownSpeed = -1;
  static s32 shownFps = -1;
  static string shownStatusRightText;
  string rightText = {};

  if(emulator && !paused) {
    auto now = chrono::millisecond();
    if(shownSpeed < 0 || now - shownAt >= 1000) {
      auto emulatedSeconds = emulatedSecondsTotal.exchange(0.0);
      auto wallSeconds = wallSecondsTotal.exchange(0.0);
      wallLastTimestampMicroseconds = 0;
      s64 emuSpeed = -1;
      if(wallSeconds > 0.0) {
        emuSpeed = (s64)(100.0 * emulatedSeconds / wallSeconds + 0.5);
      }
      auto syncWait = syncWaitEvents.exchange(0);
      if(syncWait > 0) emuSpeed = 100;
      auto frameHints = gameFrameHints.exchange(0);
      auto fps = 0;
      if(emulatedSeconds > 0.0) {
        fps = (s32)((f64)frameHints / emulatedSeconds + 0.5);
      }
      shownSpeed = emuSpeed;
      shownFps = fps;
      shownAt = now;
    }

    if(shownSpeed >= 0) rightText.append("Emu speed: ", shownSpeed, "%");
    if(shownFps >= 0)   rightText.append("  Game FPS: ", shownFps);
  } else {
    shownSpeed = -1;
    shownFps = -1;
    shownAt = 0;
    emulatedSecondsTotal = 0.0;
    wallSecondsTotal = 0.0;
    wallLastTimestampMicroseconds = 0;
    syncWaitEvents = 0;
    gameFrameHints = 0;
  }

  if(!emulator) {
    rightText = "Unloaded";
  }

  if(rightText != shownStatusRightText) {
    presentation.statusRight.setText(rightText);
    shownStatusRightText = rightText;
  }

  if (message.text == "") {
    if (emulator && keyboardCaptured) {
      presentation.statusLeft.setText("Keyboard capture is active");
    }
  }

  
  bool defocused = settings.input.defocus == "Pause" && !ruby::video.fullScreen() && !presentation.focused();
  if(emulator && defocused) message.text = "Paused";
}

auto Program::showMessage(const string& text) -> void {
  lock_guard<recursive_mutex> messageLock(_messageMutex);
  messages.push_back({chrono::millisecond(), text});
  printf("%s\n", (const char*)text);
}

auto Program::error(const string& text) -> void {
  if(kiosk) {
    fprintf(stderr, "error: %s\n", text.data());
    pendingKioskExit = true;
  } else {
    MessageDialog().setTitle("Error").setText(text).setAlignment(presentation).error();
  }
}
