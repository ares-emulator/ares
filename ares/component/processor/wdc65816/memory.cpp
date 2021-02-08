//immediate, 2-cycle opcodes with idle cycle will become bus read
//when an IRQ is to be triggered immediately after opcode completion.
//this affects the following opcodes:
//  clc, cld, cli, clv, sec, sed, sei,
//  tax, tay, txa, txy, tya, tyx,
//  tcd, tcs, tdc, tsc, tsx, txs,
//  inc, inx, iny, dec, dex, dey,
//  asl, lsr, rol, ror, nop, xce.
inline auto WDC65816::idleIRQ() -> void {
  if(interruptPending()) {
    //modify I/O cycle to bus read cycle, do not increment PC
    read(PC.d);
  } else {
    idle();
  }
}

inline auto WDC65816::idle2() -> void {
  if(D.l) idle();
}

inline auto WDC65816::idle4(n16 x, n16 y) -> void {
  if(!XF || x >> 8 != y >> 8) idle();
}

inline auto WDC65816::idle6(n16 address) -> void {
  if(EF && PC.h != address >> 8) idle();
}

inline auto WDC65816::fetch() -> n8 {
  return read(PC.b << 16 | PC.w++);
}

inline auto WDC65816::pull() -> n8 {
  EF ? (void)S.l++ : (void)S.w++;
  return read(S.w);
}

auto WDC65816::push(n8 data) -> void {
  write(S.w, data);
  EF ? (void)S.l-- : (void)S.w--;
}

inline auto WDC65816::pullN() -> n8 {
  return read(++S.w);
}

inline auto WDC65816::pushN(n8 data) -> void {
  write(S.w--, data);
}

inline auto WDC65816::readDirect(u32 address) -> n8 {
  if(EF && !D.l) return read(D.w | n8(address));
  return read(n16(D.w + address));
}

inline auto WDC65816::writeDirect(u32 address, n8 data) -> void {
  if(EF && !D.l) return write(D.w | n8(address), data);
  write(n16(D.w + address), data);
}

inline auto WDC65816::readDirectN(u32 address) -> n8 {
  return read(n16(D.w + address));
}

inline auto WDC65816::readBank(u32 address) -> n8 {
  return read((B << 16) + address);
}

inline auto WDC65816::writeBank(u32 address, n8 data) -> void {
  write((B << 16) + address, data);
}

inline auto WDC65816::readStack(u32 address) -> n8 {
  return read(n16(S.w + address));
}

inline auto WDC65816::writeStack(u32 address, n8 data) -> void {
  write(n16(S.w + address), data);
}
