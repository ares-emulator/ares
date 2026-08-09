auto POKEY::Status::power() -> void {
  value = 0xff;
}

auto POKEY::Status::resetErrors() -> void {
  value |= 0xe0;
}

auto POKEY::Status::setKeyboardDown(bool active) -> void {
  value.bit(2) = !active;
}

auto POKEY::Status::setShift(bool active) -> void {
  value.bit(3) = !active;
}

auto POKEY::Status::setKeyboardOverrun() -> void {
  value.bit(6) = 0;
}

auto POKEY::Status::setSerialInput(bool high) -> void {
  value.bit(4) = high;
}

auto POKEY::Status::setSerialBusy(bool active) -> void {
  value.bit(1) = !active;
}

auto POKEY::Status::setSerialOutput(bool high) -> void {
  value.bit(0) = high;
}

auto POKEY::Status::setSerialInputOverrun() -> void {
  value.bit(5) = 0;
}

auto POKEY::Status::setSerialFrameError() -> void {
  value.bit(7) = 0;
}

auto POKEY::Status::read() const -> n8 {
  return value;
}
