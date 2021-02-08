#if defined(Hiro_HexEdit)

auto mHexEdit::allocate() -> pObject* {
  return new pHexEdit(*this);
}

//

auto mHexEdit::address() const -> u32 {
  return state.address;
}

auto mHexEdit::backgroundColor() const -> Color {
  return state.backgroundColor;
}

auto mHexEdit::columns() const -> u32 {
  return state.columns;
}

auto mHexEdit::doRead(u32 offset) const -> u8 {
  if(state.onRead) return state.onRead(offset);
  return 0x00;
}

auto mHexEdit::doWrite(u32 offset, u8 data) const -> void {
  if(state.onWrite) return state.onWrite(offset, data);
}

auto mHexEdit::foregroundColor() const -> Color {
  return state.foregroundColor;
}

auto mHexEdit::length() const -> u32 {
  return state.length;
}

auto mHexEdit::onRead(const function<u8 (u32)>& callback) -> type& {
  state.onRead = callback;
  return *this;
}

auto mHexEdit::onWrite(const function<void (u32, u8)>& callback) -> type& {
  state.onWrite = callback;
  return *this;
}

auto mHexEdit::rows() const -> u32 {
  return state.rows;
}

auto mHexEdit::setAddress(u32 address) -> type& {
  state.address = address;
  signal(setAddress, address);
  return *this;
}

auto mHexEdit::setBackgroundColor(Color color) -> type& {
  state.backgroundColor = color;
  signal(setBackgroundColor, color);
  return *this;
}

auto mHexEdit::setColumns(u32 columns) -> type& {
  state.columns = columns;
  signal(setColumns, columns);
  return *this;
}

auto mHexEdit::setForegroundColor(Color color) -> type& {
  state.foregroundColor = color;
  signal(setForegroundColor, color);
  return *this;
}

auto mHexEdit::setLength(u32 length) -> type& {
  state.length = length;
  signal(setLength, length);
  return *this;
}

auto mHexEdit::setRows(u32 rows) -> type& {
  state.rows = rows;
  signal(setRows, rows);
  return *this;
}

auto mHexEdit::update() -> type& {
  signal(update);
  return *this;
}

#endif
