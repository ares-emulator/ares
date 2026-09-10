namespace {
auto installGdbMemoryHooks() -> void {
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
