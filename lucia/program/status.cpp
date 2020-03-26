auto Program::updateMessage() -> void {
  if(chrono::millisecond() - message.timestamp >= 2000) {
    presentation.statusLeft.setText();
  }

  if(!emulator) {
    presentation.statusRight.setText("Unloaded");
  }
}

auto Program::showMessage(const string& text) -> void {
  message.timestamp = chrono::millisecond();
  presentation.statusLeft.setText(text);
}
