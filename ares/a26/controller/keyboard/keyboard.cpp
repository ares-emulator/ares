Keyboard::Keyboard(Node::Port parent) {
  node = parent->append<Node::Peripheral>("Keyboard");

  static constexpr const char* names[12] = {
    "1", "2", "3", "4", "5", "6", "7", "8", "9", "*", "0", "#"
  };
  for(u32 index : range(12)) key[index] = node->append<Node::Input::Button>(names[index]);
}

auto Keyboard::read() -> n8 {
  n8 data = 0xff;
  data.bit(4) = readColumn(2) != ColumnState::Ground;
  return data;
}

auto Keyboard::write(n8 data) -> void {
  rows = data.bit(0, 3);
}

auto Keyboard::readAnalogA() -> AnalogConnection {
  return readAnalog(0);
}

auto Keyboard::readAnalogB() -> AnalogConnection {
  return readAnalog(1);
}

auto Keyboard::readColumn(u32 column) -> ColumnState {
  bool vcc = false;
  for(u32 row : range(4)) {
    auto& button = key[row * 3 + column];
    platform->input(button);
    if(!button->value()) continue;
    if(!rows.bit(row)) return ColumnState::Ground;
    vcc = true;
  }
  return vcc ? ColumnState::Vcc : ColumnState::Pullup;
}

auto Keyboard::readAnalog(u32 column) -> AnalogConnection {
  switch(readColumn(column)) {
  case ColumnState::Pullup: return AnalogConnection::vcc(4700);
  case ColumnState::Ground: return AnalogConnection::ground();
  case ColumnState::Vcc:    return AnalogConnection::vcc();
  }
  unreachable;
}
