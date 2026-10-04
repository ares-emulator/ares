auto Memory::power(const Linker::Result& linked) -> void {
  text = linked.text;
  rodata = linked.rodata;
  stack.assign(StackSize, 0);
  data.assign(Linker::DataCapacity, 0);
  std::copy(linked.data.begin(), linked.data.end(), data.begin());
  tables.assign(TablesSize, 0);
  for(auto& value : peripheral) value = 0;

  static const u8 ntscRows[16] = {
    0x00,0x20,0x20,0x30,0x40,0x50,0x60,0x70,0x80,0x90,0xa0,0xb0,0xc0,0xd0,0x20,0x20
  };
  static const u8 palRows[16] = {
    0x00,0x20,0x20,0x40,0x60,0x80,0xa0,0xc0,0xd0,0xb0,0x90,0x70,0x50,0x30,0x20,0x20
  };
  for(u32 value = 0; value < 256; value++) {
    tables[value] = ntscRows[value >> 4] | (value & 15);
    tables[256 + value] = palRows[value >> 4] | (value & 15);
    u8 reversed = 0;
    for(u32 bit = 0; bit < 8; bit++) reversed |= (value >> bit & 1) << (7 - bit);
    tables[512 + value] = reversed;
  }
}

auto Memory::unload() -> void {
  stack.clear();
  text.clear();
  data.clear();
  rodata.clear();
  tables.clear();
  for(auto& value : peripheral) value = 0;
}

auto Memory::get(u32 mode, n32 address, n32& value) -> ARMv6M::Fault {
  using enum ARMv6M::Fault;
  auto location = (u32)address;
  auto size = mode & ARMv6M::Byte ? 1u : mode & ARMv6M::Half ? 2u : 4u;
  auto readFrom = [&](const std::vector<u8>& source, u32 offset) -> ARMv6M::Fault {
    if(offset > source.size() || size > source.size() - offset) return Read;
    value = 0;
    for(u32 byte = 0; byte < size; byte++) value |= (u32)source[offset + byte] << byte * 8;
    return None;
  };

  if(location >= StackBase && location - StackBase < StackSize) return readFrom(stack, location - StackBase);
  if(location >= Linker::TextBase && location - Linker::TextBase < text.size()) {
    return readFrom(text, location - Linker::TextBase);
  }
  if(location >= Linker::DataBase && location - Linker::DataBase < data.size()) {
    return readFrom(data, location - Linker::DataBase);
  }
  if(location >= Linker::RodataBase && location - Linker::RodataBase < rodata.size()) {
    return readFrom(rodata, location - Linker::RodataBase);
  }
  if(location >= TablesBase && location - TablesBase < tables.size()) return readFrom(tables, location - TablesBase);
  if(location >= PeripheralBase && location - PeripheralBase + size <= sizeof(peripheral)) {
    u8 bytes[sizeof(peripheral)] = {};
    for(u32 index = 0; index < std::size(peripheral); index++) {
      for(u32 byte = 0; byte < 4; byte++) bytes[index * 4 + byte] = peripheral[index] >> byte * 8;
    }
    value = 0;
    auto offset = location - PeripheralBase;
    for(u32 byte = 0; byte < size; byte++) value |= (u32)bytes[offset + byte] << byte * 8;
    return None;
  }
  return Read;
}

auto Memory::set(u32 mode, n32 address, n32 value) -> ARMv6M::Fault {
  using enum ARMv6M::Fault;
  auto location = (u32)address;
  auto size = mode & ARMv6M::Byte ? 1u : mode & ARMv6M::Half ? 2u : 4u;
  auto writeTo = [&](std::vector<u8>& target, u32 offset) -> ARMv6M::Fault {
    if(offset > target.size() || size > target.size() - offset) return Write;
    for(u32 byte = 0; byte < size; byte++) target[offset + byte] = value >> byte * 8;
    return None;
  };

  if(location >= StackBase && location - StackBase < StackSize) return writeTo(stack, location - StackBase);
  if(location >= Linker::DataBase && location - Linker::DataBase < data.size()) {
    return writeTo(data, location - Linker::DataBase);
  }
  if(location >= PeripheralBase + 8 && location - PeripheralBase + size <= sizeof(peripheral)) {
    auto index = (location - PeripheralBase) >> 2;
    auto shift = (location & 3) * 8;
    auto mask = size == 4 ? ~0u : ((1u << size * 8) - 1) << shift;
    peripheral[index] = peripheral[index] & ~mask | ((u32)value << shift & mask);
    return None;
  }
  return Write;
}

auto Memory::writeWord(u32 address, u32 value) -> bool {
  return set(ARMv6M::Store | ARMv6M::Word, address, value) == ARMv6M::Fault::None;
}

auto Memory::serialize(serializer& s) -> void {
  for(auto& value : stack) s(value);
  for(auto& value : data) s(value);
  for(auto& value : peripheral) s(value);
}
