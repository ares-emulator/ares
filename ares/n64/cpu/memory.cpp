//32-bit segments

auto CPU::kernelSegment32(u32 address) const -> Context::Segment {
  if(address <= 0x7fff'ffff) return Context::Segment::Mapped32;  //kuseg
  if(address <= 0x9fff'ffff) return Context::Segment::Cached;    //kseg0
  if(address <= 0xbfff'ffff) return Context::Segment::Uncached;  //kseg1
  if(address <= 0xdfff'ffff) return Context::Segment::Mapped32;  //ksseg
  if(address <= 0xffff'ffff) return Context::Segment::Mapped32;  //kseg3
  unreachable;
}

auto CPU::supervisorSegment32(u32 address) const -> Context::Segment {
  if(address <= 0x7fff'ffff) return Context::Segment::Mapped32;  //suseg
  if(address <= 0xbfff'ffff) return Context::Segment::Invalid;
  if(address <= 0xdfff'ffff) return Context::Segment::Mapped32;  //sseg
  if(address <= 0xffff'ffff) return Context::Segment::Invalid;
  unreachable;
}

auto CPU::userSegment32(u32 address) const -> Context::Segment {
  if(address <= 0x7fff'ffff) return Context::Segment::Mapped32;  //useg
  if(address <= 0xffff'ffff) return Context::Segment::Invalid;
  unreachable;
}

//64-bit segments

auto CPU::kernelSegment64(u64 address) const -> Context::Segment {
  if(address <= 0x0000'00ff'ffff'ffffull) return Context::Segment::Mapped64;  //xkuseg
  if(address <= 0x3fff'ffff'ffff'ffffull) return Context::Segment::Invalid;
  if(address <= 0x4000'00ff'ffff'ffffull) return Context::Segment::Mapped64;  //xksseg
  if(address <= 0x7fff'ffff'ffff'ffffull) return Context::Segment::Invalid;
  if(address <= 0x8000'0000'ffff'ffffull) return Context::Segment::Cached;    //xkphys*
  if(address <= 0x87ff'ffff'ffff'ffffull) return Context::Segment::Invalid;
  if(address <= 0x8800'0000'ffff'ffffull) return Context::Segment::Cached;    //xkphys*
  if(address <= 0x8fff'ffff'ffff'ffffull) return Context::Segment::Invalid;
  if(address <= 0x9000'0000'ffff'ffffull) return Context::Segment::Uncached;  //xkphys*
  if(address <= 0x97ff'ffff'ffff'ffffull) return Context::Segment::Invalid;
  if(address <= 0x9800'0000'ffff'ffffull) return Context::Segment::Cached;    //xkphys*
  if(address <= 0x9fff'ffff'ffff'ffffull) return Context::Segment::Invalid;
  if(address <= 0xa000'0000'ffff'ffffull) return Context::Segment::Cached;    //xkphys*
  if(address <= 0xa7ff'ffff'ffff'ffffull) return Context::Segment::Invalid;
  if(address <= 0xa800'0000'ffff'ffffull) return Context::Segment::Cached;    //xkphys*
  if(address <= 0xafff'ffff'ffff'ffffull) return Context::Segment::Invalid;
  if(address <= 0xb000'0000'ffff'ffffull) return Context::Segment::Cached;    //xkphys*
  if(address <= 0xb7ff'ffff'ffff'ffffull) return Context::Segment::Invalid;
  if(address <= 0xb800'0000'ffff'ffffull) return Context::Segment::Cached;    //xkphys*
  if(address <= 0xbfff'ffff'ffff'ffffull) return Context::Segment::Invalid;
  if(address <= 0xc000'00ff'7fff'ffffull) return Context::Segment::Mapped64;  //xkseg
  if(address <= 0xffff'ffff'7fff'ffffull) return Context::Segment::Invalid;
  if(address <= 0xffff'ffff'9fff'ffffull) return Context::Segment::Cached;    //ckseg0
  if(address <= 0xffff'ffff'bfff'ffffull) return Context::Segment::Uncached;  //ckseg1
  if(address <= 0xffff'ffff'dfff'ffffull) return Context::Segment::Mapped64;  //ckseg2
  if(address <= 0xffff'ffff'ffff'ffffull) return Context::Segment::Mapped64;  //ckseg3
  unreachable;
}

auto CPU::supervisorSegment64(u64 address) const -> Context::Segment {
  if(address <= 0x0000'00ff'ffff'ffffull) return Context::Segment::Mapped64;  //xsuseg
  if(address <= 0x3fff'ffff'ffff'ffffull) return Context::Segment::Invalid;
  if(address <= 0x4000'00ff'ffff'ffffull) return Context::Segment::Mapped64;  //xsseg
  if(address <= 0xffff'ffff'bfff'ffffull) return Context::Segment::Invalid;
  if(address <= 0xffff'ffff'dfff'ffffull) return Context::Segment::Mapped64;  //csseg
  if(address <= 0xffff'ffff'ffff'ffffull) return Context::Segment::Invalid;
  unreachable;
}

auto CPU::userSegment64(u64 address) const -> Context::Segment {
  if(address <= 0x0000'00ff'ffff'ffffull) return Context::Segment::Mapped64;  //xuseg
  if(address <= 0xffff'ffff'ffff'ffffull) return Context::Segment::Invalid;
  unreachable;
}

//

auto CPU::devirtualize(u64 address) -> maybe<u64> {
  switch(context.segment[address >> 29 & 7]) {
  case Context::Segment::Invalid:
    exception.addressLoad();
    return nothing;
  case Context::Segment::Mapped32:
    if(auto match = tlb.load32(address)) return match.address;
    tlb.exception32(address);
    return nothing;
  case Context::Segment::Mapped64:
    if(auto match = tlb.load64(address)) return match.address;
    tlb.exception64(address);
    return nothing;
  case Context::Segment::Cached:
  case Context::Segment::Uncached:
    return address;
  }
  unreachable;
}

auto CPU::fetch(u64 address) -> u32 {
  auto segment = context.segment[address >> 29 & 7];
  if(unlikely(context.bits == 64))
  switch(segment) {
  case Context::Segment::Kernel64:     segment = kernelSegment64(address);     break;
  case Context::Segment::Supervisor64: segment = supervisorSegment64(address); break;
  case Context::Segment::User64:       segment = userSegment64(address);       break;
  }

  switch(segment) {
  case Context::Segment::Invalid:
    step(1);
    exception.addressLoad();
    return 0;  //nop
  case Context::Segment::Mapped32:
    if(auto match = tlb.load32(address)) {
      if(match.cache) return icache.fetch(match.address);
      step(1);
      return bus.read<Word>(match.address);
    }
    step(1);
    tlb.exception32(address);
    return 0;  //nop
  case Context::Segment::Mapped64:
    if(auto match = tlb.load64(address)) {
      if(match.cache) return icache.fetch(match.address);
      step(1);
      return bus.read<Word>(match.address);
    }
    step(1);
    tlb.exception64(address);
    return 0;  //nop
  case Context::Segment::Cached:
    return icache.fetch(address);
  case Context::Segment::Uncached:
    step(1);
    return bus.read<Word>(address);
  }

  unreachable;
}

template<u32 Size>
auto CPU::read(u64 address) -> maybe<u64> {
  if constexpr(Accuracy::CPU::AddressErrors) {
    if(unlikely(address & Size - 1)) {
      step(1);
      exception.addressLoad();
      return nothing;
    }
  }

  auto segment = context.segment[address >> 29 & 7];
  if(unlikely(context.bits == 64))
  switch(segment) {
  case Context::Segment::Kernel64:     segment = kernelSegment64(address);     break;
  case Context::Segment::Supervisor64: segment = supervisorSegment64(address); break;
  case Context::Segment::User64:       segment = userSegment64(address);       break;
  }

  switch(segment) {
  case Context::Segment::Invalid:
    step(1);
    exception.addressLoad();
    return nothing;
  case Context::Segment::Mapped32:
    if(auto match = tlb.load32(address)) {
      if(match.cache) return dcache.read<Size>(match.address);
      step(1);
      return bus.read<Size>(match.address);
    }
    step(1);
    tlb.exception32(address);
    return nothing;
  case Context::Segment::Mapped64:
    if(auto match = tlb.load64(address)) {
      if(match.cache) return dcache.read<Size>(match.address);
      step(1);
      return bus.read<Size>(match.address);
    }
    step(1);
    tlb.exception64(address);
    return nothing;
  case Context::Segment::Cached:
    return dcache.read<Size>(address);
  case Context::Segment::Uncached:
    step(1);
    return bus.read<Size>(address);
  }

  unreachable;
}

template<u32 Size>
auto CPU::write(u64 address, u64 data) -> bool {
  if constexpr(Accuracy::CPU::AddressErrors) {
    if(unlikely(address & Size - 1)) {
      step(1);
      exception.addressStore();
      return false;
    }
  }

  auto segment = context.segment[address >> 29 & 7];
  if(unlikely(context.bits == 64))
  switch(segment) {
  case Context::Segment::Kernel64:     segment = kernelSegment64(address);     break;
  case Context::Segment::Supervisor64: segment = supervisorSegment64(address); break;
  case Context::Segment::User64:       segment = userSegment64(address);       break;
  }

  switch(segment) {
  case Context::Segment::Invalid:
    step(1);
    exception.addressStore();
    return false;
  case Context::Segment::Mapped32:
    if(auto match = tlb.store32(address)) {
      if(match.cache) return dcache.write<Size>(match.address, data), true;
      step(1);
      return bus.write<Size>(match.address, data), true;
    }
    step(1);
    tlb.exception32(address);
    return false;
  case Context::Segment::Mapped64:
    if(auto match = tlb.store64(address)) {
      if(match.cache) return dcache.write<Size>(match.address, data), true;
      step(1);
      return bus.write<Size>(match.address, data), true;
    }
    step(1);
    tlb.exception64(address);
    return false;
  case Context::Segment::Cached:
    return dcache.write<Size>(address, data), true;
  case Context::Segment::Uncached:
    step(1);
    return bus.write<Size>(address, data), true;
  }

  unreachable;
}
