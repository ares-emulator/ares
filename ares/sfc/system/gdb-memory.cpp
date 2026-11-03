namespace {
auto installGdbMemoryHooks() -> void {
  nall::GDB::server.hooks.checkedWrite = [](u64 address, const std::vector<u8>& bytes) -> string {
    if(!nall::GDB::server.isHalted()) return "E09";
    // Physical WRAM writes bypass MMIO, CPU timing and interrupt machinery.
    // Validate the whole range first so a crossing write leaves every byte unchanged.
    if(address < 0x7e0000 || address > 0x800000 || bytes.size() > 0x800000 - address) return "E00";
    for(size_t i = 0; i < bytes.size(); i++) cpu.wram[address - 0x7e0000 + i] = bytes[i];
    return "OK";
  };
  nall::GDB::server.hooks.read = [](u64 address, u32 size) -> string {
    // RSP m reads are exact. We do not alias invalid addresses or synthesize MMIO.
    if(address > 0xff'ffff || size > 0x100'0000 - address || size > 0x2000) return "E00";
    string result;
    result.reserve(size * 2);
    for(u32 byte : range(size)) {
      auto value = bus.peek(address + byte);
      if(!value) return "E00";
      result.append(hex(value.get(), 2, '0'));
    }
    return result;
  };
}
}
