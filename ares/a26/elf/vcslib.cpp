auto externalSymbols(bool ntsc) -> std::vector<Linker::External> {
  static const char* names[] = {
    "memset", "memcpy", "vcsLdaForBusStuff2", "vcsLdxForBusStuff2", "vcsLdyForBusStuff2",
    "vcsWrite3", "vcsJmp3", "vcsNop2", "vcsNop2n", "vcsWrite5", "vcsWrite6", "vcsLda2",
    "vcsLdx2", "vcsLdy2", "vcsSax3", "vcsSta3", "vcsStx3", "vcsSty3", "vcsSta4", "vcsStx4",
    "vcsSty4", "vcsCopyOverblankToRiotRam", "vcsStartOverblank", "vcsEndOverblank", "vcsRead4",
    "randint", "vcsTxs2", "vcsJsr6", "vcsPha3", "vcsPhp3", "vcsPla4", "vcsPlp4", "vcsPla4Ex",
    "vcsPlp4Ex", "vcsJmpToRam3", "vcsWaitForAddress", "injectDmaData", "vcsWrite4"
  };

  std::vector<Linker::External> symbols;
  symbols.push_back({"ADDR_IDR", Memory::PeripheralBase + 1});
  symbols.push_back({"DATA_IDR", Memory::PeripheralBase + 5});
  symbols.push_back({"DATA_ODR", Memory::PeripheralBase + 9});
  symbols.push_back({"DATA_MODER", Memory::PeripheralBase + 13});
  for(u32 index = 0; index < std::size(names); index++) {
    symbols.push_back({names[index], Memory::StubBase + index * 4 + 1});
  }
  symbols.push_back({"ColorLookup", Memory::TablesBase + (ntsc ? 0 : 256)});
  symbols.push_back({"ReverseByte", Memory::TablesBase + 512});
  return symbols;
}

auto VCSLib::power() -> void {
  queue.reset();
  busDriven = false;
  busValue = 0;
  currentAddress = 0;
  currentData = 0;
  waiting = false;
  waitingAddress = 0;
  stuffMaskA = 0;
  stuffMaskX = 0;
  stuffMaskY = 0;
  randomState = 1;
  busAccessCounter = 0;
  bootstrap();
}

auto VCSLib::access(n16 address, n8 value, Memory& memory) -> n8 {
  auto timestamp = ++busAccessCounter;
  BusQueue::Transaction transaction;
  if(queue.consume(address, timestamp, transaction)) {
    busDriven = !transaction.yield;
    busValue = transaction.data;
  }
  if(busDriven) value = busValue;
  currentAddress = address & 0x1fff;
  currentData = value;
  memory.peripheral[0] = currentAddress;
  memory.peripheral[1] = currentData;
  return value;
}

auto VCSLib::stalled() const -> bool {
  return waiting && (!queue.empty() || currentAddress != waitingAddress);
}

auto VCSLib::execute(u32 address, ARMv6M& arm, Memory& memory) -> ARMv6M::Fault {
  using enum ARMv6M::Fault;
  if(queue.size() >= 10) return Suspend;
  queue.setTimestamp(busAccessCounter);
  auto argument = (u32)arm.r(0);
  auto value = (u8)arm.r(1);
  stuffMaskA = memory.peripheral[2];
  stuffMaskX = memory.peripheral[2] >> 8;
  stuffMaskY = memory.peripheral[2] >> 16;

  switch(address) {
  case Memory::StubBase + 0: {
    auto target = (u32)arm.r(0), size = (u32)arm.r(2);
    for(u32 offset = 0; offset < size; offset++) {
      if(auto error = memory.set(ARMv6M::Store | ARMv6M::Byte, target + offset, value); error != None) {
        return error;
      }
    }
    arm.r(0) = target;
    return None;
  }
  case Memory::StubBase + 4: {
    auto target = (u32)arm.r(0), source = (u32)arm.r(1), size = (u32)arm.r(2);
    for(u32 offset = 0; offset < size; offset++) {
      n32 byte = 0;
      if(auto error = memory.get(ARMv6M::Load | ARMv6M::Byte, source + offset, byte); error != None) return error;
      if(auto error = memory.set(ARMv6M::Store | ARMv6M::Byte, target + offset, byte); error != None) return error;
    }
    arm.r(0) = target;
    return None;
  }
  case Memory::StubBase + 8: {
    queue.inject(0xa9);
    queue.inject(stuffMaskA);
    return None;
  }
  case Memory::StubBase + 12: {
    queue.inject(0xa2);
    queue.inject(stuffMaskX);
    return None;
  }
  case Memory::StubBase + 16: {
    queue.inject(0xa0);
    queue.inject(stuffMaskY);
    return None;
  }
  case Memory::StubBase + 20: {
    queue.inject(0x85);
    queue.inject(argument);
    queue.stuff(argument, value);
    return None;
  }
  case Memory::StubBase + 24: {
    queue.inject(0x4c);
    queue.inject(0);
    queue.inject(0x10);
    queue.setNextAddress(0x1000);
    return None;
  }
  case Memory::StubBase + 28: {
    queue.inject(0xea);
    return None;
  }
  case Memory::StubBase + 32: {
    if(argument) {
      queue.inject(0xea);
      queue.setNextAddress(queue.getNextAddress() + argument - 1);
    }
    return None;
  }
  case Memory::StubBase + 36: {
    write5(argument, value);
    return None;
  }
  case Memory::StubBase + 40: {
    queue.inject(0xa9);
    queue.inject(value);
    queue.inject(0x8d);
    queue.inject(argument);
    queue.inject(argument >> 8);
    queue.release(argument);
    return None;
  }
  case Memory::StubBase + 44: {
    queue.inject(0xa9);
    queue.inject(argument);
    return None;
  }
  case Memory::StubBase + 48: {
    queue.inject(0xa2);
    queue.inject(argument);
    return None;
  }
  case Memory::StubBase + 52: {
    queue.inject(0xa0);
    queue.inject(argument);
    return None;
  }
  case Memory::StubBase + 56: {
    queue.inject(0x87);
    queue.inject(argument);
    queue.release(argument);
    return None;
  }
  case Memory::StubBase + 60: {
    queue.inject(0x85);
    queue.inject(argument);
    queue.release(argument);
    return None;
  }
  case Memory::StubBase + 64: {
    queue.inject(0x86);
    queue.inject(argument);
    queue.release(argument);
    return None;
  }
  case Memory::StubBase + 68: {
    queue.inject(0x84);
    queue.inject(argument);
    queue.release(argument);
    return None;
  }
  case Memory::StubBase + 72: {
    queue.inject(0x8d);
    queue.inject(argument);
    queue.inject(argument >> 8);
    queue.release(argument);
    return None;
  }
  case Memory::StubBase + 76: {
    queue.inject(0x8e);
    queue.inject(argument);
    queue.inject(argument >> 8);
    queue.release(argument);
    return None;
  }
  case Memory::StubBase + 80: {
    queue.inject(0x8c);
    queue.inject(argument);
    queue.inject(argument >> 8);
    queue.release(argument);
    return None;
  }
  case Memory::StubBase + 84: {
    copyOverblank();
    return None;
  }
  case Memory::StubBase + 88: {
    queue.inject(0x4c);
    queue.inject(0x80);
    queue.inject(0);
    queue.release(0x80);
    return None;
  }
  case Memory::StubBase + 92: {
    queue.injectAt(0x1fff, 0);
    queue.release(0xac);
    queue.setNextAddress(0x1000);
    return None;
  }
  case Memory::StubBase + 96: {
    if(waiting) {
      if(stalled()) return Suspend;
      waiting = false;
      arm.r(0) = currentData;
      return None;
    }
    waiting = true;
    waitingAddress = argument & 0x1fff;
    queue.inject(0xad);
    queue.inject(argument);
    queue.inject(argument >> 8);
    queue.release(argument);
    return Suspend;
  }
  case Memory::StubBase + 100: {
    randomState = randomState * 1664525 + 1013904223;
    arm.r(0) = randomState;
    return None;
  }
  case Memory::StubBase + 104: {
    queue.inject(0x9a);
    return None;
  }
  case Memory::StubBase + 108: {
    queue.inject(0x20);
    queue.inject(argument);
    queue.release(0, 0x1000);
    queue.inject(argument >> 8);
    queue.setNextAddress(argument);
    return None;
  }
  case Memory::StubBase + 112: {
    queue.inject(0x48);
    queue.release(0, 0x1000);
    return None;
  }
  case Memory::StubBase + 116: {
    queue.inject(0x08);
    queue.release(0, 0x1000);
    return None;
  }
  case Memory::StubBase + 120: {
    queue.inject(0x68);
    queue.release(0, 0x1000);
    return None;
  }
  case Memory::StubBase + 124: {
    queue.inject(0x28);
    queue.release(0, 0x1000);
    return None;
  }
  case Memory::StubBase + 136: {
    queue.inject(0x4c);
    queue.inject(argument);
    queue.inject(argument >> 8);
    queue.release(argument);
    return None;
  }
  case Memory::StubBase + 148: {
    queue.inject(0x8d);
    queue.inject(argument);
    queue.inject(argument >> 8);
    queue.stuff(argument, value);
    return None;
  }
  default: {
    return Undefined;
  }
  }
}

auto VCSLib::bootstrap() -> void {
  queue.injectAt(0x1ffc, 0x00);
  queue.inject(0x10);
  queue.setNextAddress(0x1000);
  copyOverblank();
  queue.inject(0x4c); queue.inject(0x80); queue.inject(0x00); queue.release(0x0080);
  queue.injectAt(0x1fff, 0x00); queue.release(0x00ac); queue.setNextAddress(0x1000);
  queue.inject(0xea);
  queue.setNextAddress(queue.getNextAddress() + 1023);
}

auto VCSLib::write5(u16 address, u8 value) -> void {
  queue.inject(0xa9); queue.inject(value); queue.inject(0x85); queue.inject(address); queue.release(address);
}

auto VCSLib::copyOverblank() -> void {
  static const u8 program[] = {
    0xa0,0x00,0xa5,0xe0,0x85,0x02,0x85,0x2d,0x98,0x18,0x6a,0xaa,0xb5,0xe0,0x90,0x04,
    0x4a,0x4a,0x4a,0x4a,0xc8,0xc0,0x1d,0xd0,0x04,0xa2,0x02,0x86,0x00,0xc0,0x20,0xd0,
    0x04,0xa2,0x00,0x86,0x00,0xc0,0x3f,0xd0,0xdb,0xae,0xff,0xff,0xd0,0xfb,0x4c,0x00,0x10
  };
  for(u32 index = 0; index < std::size(program); index++) write5(0x80 + index, program[index]);
}

auto VCSLib::serialize(serializer& s) -> void {
  queue.serialize(s);
  s(busDriven);
  s(busValue);
  s(currentAddress);
  s(currentData);
  s(waiting);
  s(waitingAddress);
  s(stuffMaskA);
  s(stuffMaskX);
  s(stuffMaskY);
  s(randomState);
  s(busAccessCounter);
}
