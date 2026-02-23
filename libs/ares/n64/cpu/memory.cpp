//32-bit segments

auto CPU::kernelSegment32(u32 vaddr) const -> Context::Segment {
  if(vaddr <= 0x7fff'ffff) return Context::Segment::Mapped;  //kuseg
  if(vaddr <= 0x9fff'ffff) return Context::Segment::Cached;  //kseg0
  if(vaddr <= 0xbfff'ffff) return Context::Segment::Direct;  //kseg1
  if(vaddr <= 0xdfff'ffff) return Context::Segment::Mapped;  //ksseg
  if(vaddr <= 0xffff'ffff) return Context::Segment::Mapped;  //kseg3
  unreachable;
}

auto CPU::supervisorSegment32(u32 vaddr) const -> Context::Segment {
  if(vaddr <= 0x7fff'ffff) return Context::Segment::Mapped;  //suseg
  if(vaddr <= 0xbfff'ffff) return Context::Segment::Unused;
  if(vaddr <= 0xdfff'ffff) return Context::Segment::Mapped;  //sseg
  if(vaddr <= 0xffff'ffff) return Context::Segment::Unused;
  unreachable;
}

auto CPU::userSegment32(u32 vaddr) const -> Context::Segment {
  if(vaddr <= 0x7fff'ffff) return Context::Segment::Mapped;  //useg
  if(vaddr <= 0xffff'ffff) return Context::Segment::Unused;
  unreachable;
}

//64-bit segments

auto CPU::kernelSegment64(u64 vaddr) const -> Context::Segment {
  if(vaddr <= 0x0000'00ff'ffff'ffffull) return Context::Segment::Mapped;  //xkuseg
  if(vaddr <= 0x3fff'ffff'ffff'ffffull) return Context::Segment::Unused;
  if(vaddr <= 0x4000'00ff'ffff'ffffull) return Context::Segment::Mapped;  //xksseg
  if(vaddr <= 0x7fff'ffff'ffff'ffffull) return Context::Segment::Unused;
  if(vaddr <= 0x8000'0000'ffff'ffffull) return Context::Segment::Cached32;  //xkphys*
  if(vaddr <= 0x87ff'ffff'ffff'ffffull) return Context::Segment::Unused;
  if(vaddr <= 0x8800'0000'ffff'ffffull) return Context::Segment::Cached32;  //xkphys*
  if(vaddr <= 0x8fff'ffff'ffff'ffffull) return Context::Segment::Unused;
  if(vaddr <= 0x9000'0000'ffff'ffffull) return Context::Segment::Direct32;  //xkphys*
  if(vaddr <= 0x97ff'ffff'ffff'ffffull) return Context::Segment::Unused;
  if(vaddr <= 0x9800'0000'ffff'ffffull) return Context::Segment::Cached32;  //xkphys*
  if(vaddr <= 0x9fff'ffff'ffff'ffffull) return Context::Segment::Unused;
  if(vaddr <= 0xa000'0000'ffff'ffffull) return Context::Segment::Cached32;  //xkphys*
  if(vaddr <= 0xa7ff'ffff'ffff'ffffull) return Context::Segment::Unused;
  if(vaddr <= 0xa800'0000'ffff'ffffull) return Context::Segment::Cached32;  //xkphys*
  if(vaddr <= 0xafff'ffff'ffff'ffffull) return Context::Segment::Unused;
  if(vaddr <= 0xb000'0000'ffff'ffffull) return Context::Segment::Cached32;  //xkphys*
  if(vaddr <= 0xb7ff'ffff'ffff'ffffull) return Context::Segment::Unused;
  if(vaddr <= 0xb800'0000'ffff'ffffull) return Context::Segment::Cached32;  //xkphys*
  if(vaddr <= 0xbfff'ffff'ffff'ffffull) return Context::Segment::Unused;
  if(vaddr <= 0xc000'00ff'7fff'ffffull) return Context::Segment::Mapped;  //xkseg
  if(vaddr <= 0xffff'ffff'7fff'ffffull) return Context::Segment::Unused;
  if(vaddr <= 0xffff'ffff'9fff'ffffull) return Context::Segment::Cached;  //ckseg0
  if(vaddr <= 0xffff'ffff'bfff'ffffull) return Context::Segment::Direct;  //ckseg1
  if(vaddr <= 0xffff'ffff'dfff'ffffull) return Context::Segment::Mapped;  //ckseg2
  if(vaddr <= 0xffff'ffff'ffff'ffffull) return Context::Segment::Mapped;  //ckseg3
  unreachable;
}

auto CPU::supervisorSegment64(u64 vaddr) const -> Context::Segment {
  if(vaddr <= 0x0000'00ff'ffff'ffffull) return Context::Segment::Mapped;  //xsuseg
  if(vaddr <= 0x3fff'ffff'ffff'ffffull) return Context::Segment::Unused;
  if(vaddr <= 0x4000'00ff'ffff'ffffull) return Context::Segment::Mapped;  //xsseg
  if(vaddr <= 0xffff'ffff'bfff'ffffull) return Context::Segment::Unused;
  if(vaddr <= 0xffff'ffff'dfff'ffffull) return Context::Segment::Mapped;  //csseg
  if(vaddr <= 0xffff'ffff'ffff'ffffull) return Context::Segment::Unused;
  unreachable;
}

auto CPU::userSegment64(u64 vaddr) const -> Context::Segment {
  if(vaddr <= 0x0000'00ff'ffff'ffffull) return Context::Segment::Mapped;  //xuseg
  if(vaddr <= 0xffff'ffff'ffff'ffffull) return Context::Segment::Unused;
  unreachable;
}

//

auto CPU::segment(u64 vaddr) -> Context::Segment {
  auto segment = context.segment[vaddr >> 29 & 7];
  if(likely(context.bits == 32))
    return (Context::Segment)segment;
  switch(segment) {
  case Context::Segment::Kernel64:
    return kernelSegment64(vaddr);
  case Context::Segment::Supervisor64:
    return supervisorSegment64(vaddr);
  case Context::Segment::User64:
    return userSegment64(vaddr);
  }
  unreachable;
}

template <u32 Dir, u32 Size>
auto CPU::devirtualize(u64 vaddr, bool raiseAlignedError, bool raiseExceptions) -> PhysAccess {
  if (raiseAlignedError && vaddrAlignedError<Size>(vaddr, Dir == Write)) {
    return PhysAccess{false};
  }
  //fast path for RDRAM, which is by far the most accessed memory region
  if (vaddr >= 0xffff'ffff'8000'0000ull && vaddr <= 0xffff'ffff'83ef'ffffull) {
    if constexpr(Dir == Read)  return PhysAccess{true, true, (u32)vaddr & 0x3eff'ffff, vaddr};
    if constexpr(Dir == Write) return PhysAccess{true, true, (u32)vaddr & 0x3eff'ffff, vaddr};
  }
  switch(segment(vaddr)) {
  case Context::Segment::Unused:
    if(raiseExceptions) {
      addressException(vaddr);
      if constexpr(Dir == Read)  exception.addressLoad();
      if constexpr(Dir == Write) exception.addressStore();
    }
    return PhysAccess{false};
  case Context::Segment::Mapped:
    if constexpr(Dir == Read)  if(auto access = tlb.load (vaddr, !raiseExceptions)) return access;
    if constexpr(Dir == Write) if(auto access = tlb.store(vaddr, !raiseExceptions)) return access;
    return PhysAccess{false};
  case Context::Segment::Cached:
    return PhysAccess{true, true,  (u32)(vaddr & 0x1fff'ffff), vaddr};
  case Context::Segment::Direct:
    return PhysAccess{true, false, (u32)(vaddr & 0x1fff'ffff), vaddr};
  case Context::Segment::Cached32:
    return PhysAccess{true, true,  (u32)(vaddr & 0xffff'ffff), vaddr};
  case Context::Segment::Direct32:
    return PhysAccess{true, false,  (u32)(vaddr & 0xffff'ffff), vaddr};
  }
  unreachable;
}

auto CPU::devirtualizeDebug(u64 vaddr) -> u64 {
  return devirtualize<Read, Byte>(vaddr, false).paddr; // this wrapper preserves the inlining of 'devirtualizeFast'
}

template<u32 Size>
inline auto CPU::busWrite(u32 address, u64 data) -> void {
  bus.write<Size>(address, data, *this, "CPU");
}

template<u32 Size>
inline auto CPU::busWriteBurst(u32 address, u32 *data) -> void {
  bus.writeBurst<Size>(address, data, *this);
}

template<u32 Size>
inline auto CPU::busRead(u32 address) -> u64 {
  return bus.read<Size>(address, *this, "CPU");
}

template<u32 Size>
inline auto CPU::busReadBurst(u32 address, u32 *data) -> void {
  return bus.readBurst<Size>(address, data, *this);
}

auto CPU::fetch(PhysAccess access) -> maybe<u32> {
  step(1 * 2);
  if(!access) return nothing;
  if(access.cache) return icache.fetch(access.vaddr, access.paddr, cpu);
  return busRead<Word>(access.paddr);
}

template<u32 Size>
auto CPU::read(PhysAccess access) -> maybe<u64> {
  if(!access) return nothing;
  GDB::server.reportMemRead(access.vaddr, Size);
  if(access.cache) return dcache.read<Size>(access.vaddr, access.paddr);
  return busRead<Size>(access.paddr);
}

template<u32 Size>
auto CPU::readDebug(u64 vaddr) -> u64 {
  Thread dummyThread{};
  auto access = devirtualize<Read, Size>(vaddr, false, false);
  if(!access) return 0;
  if(access.cache) return dcache.readDebug<Size>(access.vaddr, access.paddr);
  return bus.read<Size>(access.paddr, dummyThread, "Ares Debugger");
}


template<u32 Size>
auto CPU::write(PhysAccess access, u64 data) -> bool {
  if(!access) return false;
  GDB::server.reportMemWrite(access.vaddr, Size);
  if(access.cache) return dcache.write<Size>(access.vaddr, access.paddr, data), true;
  return busWrite<Size>(access.paddr, data), true;
}

template<u32 Size>
auto CPU::writeDebug(u64 vaddr, u64 data) -> bool {
  Thread dummyThread{};
  auto access = devirtualize<Write, Size>(vaddr, false, false);
  if(!access) return false;
  GDB::server.reportMemWrite(access.vaddr, Size);
  if(access.cache) return dcache.writeDebug<Size>(access.vaddr, access.paddr, data), true;
  return bus.write<Size>(access.paddr, data, dummyThread, "Ares Debugger"), true;
}

template<u32 Size>
auto CPU::vaddrAlignedError(u64 vaddr, bool write) -> bool {
  if constexpr(Accuracy::CPU::AddressErrors) {
    if(unlikely(vaddr & Size - 1)) {
      step(1 * 2);
      addressException(vaddr);
      if(write) exception.addressStore();
      else exception.addressLoad();
      return true;
    }
    if (context.bits == 32 && unlikely((s32)vaddr != vaddr)) {
      step(1 * 2);
      addressException(vaddr);
      if(write) exception.addressStore();
      else exception.addressLoad();
      return true;
    }
  }
  return false;
}

auto CPU::addressException(u64 vaddr) -> void {
  scc.badVirtualAddress = vaddr;
  scc.tlb.virtualAddress.bit(13,39) = vaddr >> 13;
  scc.tlb.region = vaddr >> 62;
  scc.context.badVirtualAddress = vaddr >> 13;
  scc.xcontext.badVirtualAddress = vaddr >> 13;
  scc.xcontext.region = vaddr >> 62;
}

template auto CPU::writeDebug<Byte>(u64, u64) -> bool;
template auto CPU::writeDebug<Half>(u64, u64) -> bool;
template auto CPU::writeDebug<Word>(u64, u64) -> bool;
template auto CPU::writeDebug<Dual>(u64, u64) -> bool;
template auto CPU::readDebug<Byte>(u64) -> u64;
template auto CPU::readDebug<Half>(u64) -> u64;
template auto CPU::readDebug<Word>(u64) -> u64;
template auto CPU::readDebug<Dual>(u64) -> u64;
