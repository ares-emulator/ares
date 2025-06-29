auto Program::updateMessage() -> void {
  {
    // This function is called every iteration of the GUI run loop. Acquiring the emulator mutex would incur a severe
    // responsiveness penalty, so use a dedicated mutex for message passing.
    lock_guard<recursive_mutex> messageLock(messageMutex);
    if(chrono::millisecond() - message.timestamp >= 2000) {
      message = {};
      if(messages.size()) message = messages.takeFirst();
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
  }

  if(vblanksPerSecond > 0) {
    presentation.statusRight.setText({vblanksPerSecond.load(), " VPS"});
  }

  if(!emulator) {
    presentation.statusRight.setText("Unloaded");
  }

  if (message.text == "") {
    if (emulator && keyboardCaptured) {
      presentation.statusLeft.setText("Keyboard capture is active");
    }
  }

  
  bool defocused = driverSettings.inputDefocusPause.checked() && !ruby::video.fullScreen() && !presentation.focused();
  if(emulator && defocused) message.text = "Paused";
}

auto Program::showMessage(const string& text) -> void {
  lock_guard<recursive_mutex> messageLock(messageMutex);
  messages.append({chrono::millisecond(), text});
  printf("%s\n", (const char*)text);
}
