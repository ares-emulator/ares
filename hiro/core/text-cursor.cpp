#if defined(Hiro_TextCursor)

TextCursor::TextCursor(s32 offset, s32 length) {
  setTextCursor(offset, length);
}

TextCursor::operator bool() const {
  return offset() || length();
}

auto TextCursor::operator==(const TextCursor& source) const -> bool {
  return offset() == source.offset() && length() == source.length();
}

auto TextCursor::operator!=(const TextCursor& source) const -> bool {
  return !operator==(source);
}

auto TextCursor::length() const -> s32 {
  return state.length;
}

auto TextCursor::offset() const -> s32 {
  return state.offset;
}

auto TextCursor::setLength(s32 length) -> type& {
  state.length = length;
  return *this;
}

auto TextCursor::setOffset(s32 offset) -> type& {
  state.offset = offset;
  return *this;
}

auto TextCursor::setTextCursor(s32 offset, s32 length) -> type& {
  state.offset = offset;
  state.length = length;
  return *this;
}

#endif
