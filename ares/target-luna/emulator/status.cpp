auto Emulator::updateMessage() -> void {
  if(chrono::millisecond() - state.message.timestamp >= 2000) {
    program.statusMessage.setText(state.message.text);
  }
}

auto Emulator::showMessage(const string& message) -> void {
  state.message.timestamp = chrono::millisecond();
  program.statusMessage.setText(message);
}

auto Emulator::setCaption(const string& caption) -> void {
  program.statusCaption.setText(caption);
}
