auto PCD::BRAM::load(Node::Object parent) -> void {
  memory.allocate(2_KiB);
}

auto PCD::BRAM::unload() -> void {
  memory.reset();
}

auto PCD::BRAM::read(uint11 address) -> uint8 {
  return memory.read(address);
}

auto PCD::BRAM::write(uint11 address, uint8 data) -> void {
  if(!writable) return;
  return memory.write(address, data);
}
