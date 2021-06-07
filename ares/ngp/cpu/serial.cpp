auto CPU::SerialChannel::receive() -> n8 {
  return buffer;
}

auto CPU::SerialChannel::transmit(n8 data) -> void {
  buffer = data;
}
