#if defined(Hiro_Hotkey)

Hotkey::Hotkey() : state(new Hotkey::State) {
  setSequence();
}

Hotkey::Hotkey(const string& sequence) : state(new Hotkey::State) {
  setSequence(sequence);
}

Hotkey::operator bool() const {
  return (bool)state->sequence;
}

auto Hotkey::operator==(const Hotkey& source) const -> bool {
  return state == source.state;
}

auto Hotkey::operator!=(const Hotkey& source) const -> bool {
  return !operator==(source);
}

auto Hotkey::doPress() const -> void {
  if(state->onPress) state->onPress();
}

auto Hotkey::doRelease() const -> void {
  if(state->onRelease) state->onRelease();
}

auto Hotkey::onPress(const function<void ()>& callback) -> type& {
  state->onPress = callback;
  return *this;
}

auto Hotkey::onRelease(const function<void ()>& callback) -> type& {
  state->onRelease = callback;
  return *this;
}

auto Hotkey::reset() -> type& {
  setSequence();
  return *this;
}

auto Hotkey::sequence() const -> string {
  return state->sequence;
}

auto Hotkey::setSequence(const string& sequence) -> type& {
  state->active = false;
  state->sequence = sequence;
  state->keys.clear();
  for(auto& key : sequence.split("+")) {
    if (auto idx = nall::index_of(Keyboard::keys, key)) state->keys.push_back((u32)*idx);
  }
  return *this;
}

#endif
