auto CPU::devirtualize(u32 address) -> maybe<u32> {
  switch(context.segment[address >> 29]) {
  case Context::Segment::Invalid:
    exception.addressLoad();
    return nothing;
  case Context::Segment::Mapped:
    if(auto match = tlb.load(address)) return match.address;
    tlb.exception(address);
    return nothing;
  case Context::Segment::Cached:
  case Context::Segment::Uncached:
    return address;
  }
  unreachable;
}

auto CPU::fetch(u32 address) -> u32 {
  switch(context.segment[address >> 29]) {
  case Context::Segment::Invalid:
    step(1);
    exception.addressLoad();
    return 0;  //nop
  case Context::Segment::Mapped:
    if(auto match = tlb.load(address)) {
      if(match.cache) return icache.fetch(match.address);
      step(1);
      return bus.read<Word>(match.address);
    }
    step(1);
    tlb.exception(address);
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
auto CPU::read(u32 address) -> maybe<u64> {
  if constexpr(Accuracy::CPU::AddressErrors) {
    if(unlikely(address & Size - 1)) {
      step(1);
      exception.addressLoad();
      return nothing;
    }
  }

  switch(context.segment[address >> 29]) {
  case Context::Segment::Invalid:
    step(1);
    exception.addressLoad();
    return nothing;
  case Context::Segment::Mapped:
    if(auto match = tlb.load(address)) {
      if(match.cache) return dcache.read<Size>(match.address);
      step(1);
      return bus.read<Size>(match.address);
    }
    step(1);
    tlb.exception(address);
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
auto CPU::write(u32 address, u64 data) -> bool {
  if constexpr(Accuracy::CPU::AddressErrors) {
    if(unlikely(address & Size - 1)) {
      step(1);
      exception.addressStore();
      return false;
    }
  }

  switch(context.segment[address >> 29]) {
  case Context::Segment::Invalid:
    step(1);
    exception.addressStore();
    return false;
  case Context::Segment::Mapped:
    if(auto match = tlb.store(address)) {
      if(match.cache) return dcache.write<Size>(match.address, data), true;
      step(1);
      return bus.write<Size>(match.address, data), true;
    }
    step(1);
    tlb.exception(address);
    return false;
  case Context::Segment::Cached:
    return dcache.write<Size>(address, data), true;
  case Context::Segment::Uncached:
    step(1);
    return bus.write<Size>(address, data), true;
  }
  unreachable;
}
