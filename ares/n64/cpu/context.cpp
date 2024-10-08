auto CPU::Context::setMode() -> void {
  mode = min(2, self.scc.status.privilegeMode);
  if(self.scc.status.exceptionLevel) mode = Mode::Kernel;
  if(self.scc.status.errorLevel) mode = Mode::Kernel;

  switch(mode) {
  case Mode::Kernel:
    endian = self.scc.configuration.bigEndian;
    bits = self.scc.status.kernelExtendedAddressing ? 64 : 32;
    break;
  case Mode::Supervisor:
    endian = self.scc.configuration.bigEndian;
    bits = self.scc.status.supervisorExtendedAddressing ? 64 : 32;
    break;
  case Mode::User:
    endian = self.scc.configuration.bigEndian ^ self.scc.status.reverseEndian;
    bits = self.scc.status.userExtendedAddressing ? 64 : 32;
    break;
  }

  jit.update(*this, self);
  jitBits = jit.toBits();

  if(bits == 32) {
    physMask = 0x1fff'ffff;
    segment[0] = Segment::Mapped;
    segment[1] = Segment::Mapped;
    segment[2] = Segment::Mapped;
    segment[3] = Segment::Mapped;
    switch(mode) {
    case Mode::Kernel:
      segment[4] = Segment::Cached;
      segment[5] = Segment::Direct;
      segment[6] = Segment::Mapped;
      segment[7] = Segment::Mapped;
      break;
    case Mode::Supervisor:
      segment[4] = Segment::Unused;
      segment[5] = Segment::Unused;
      segment[6] = Segment::Mapped;
      segment[7] = Segment::Unused;
      break;
    case Mode::User:
      segment[4] = Segment::Unused;
      segment[5] = Segment::Unused;
      segment[6] = Segment::Unused;
      segment[7] = Segment::Unused;
      break;
    }
    return;
  }

  if(bits == 64) {
    physMask = 0x7fff'ffff;
    for(auto n : range(8))
    switch(mode) {
    case Mode::Kernel:
      segment[n] = Segment::Kernel64;
      break;
    case Mode::Supervisor:
      segment[n] = Segment::Supervisor64;
      break;
    case Mode::User:
      segment[n] = Segment::User64;
      break;
    }
  }
}

auto CPU::Context::JIT::update(const Context& ctx, const CPU& cpu) -> void {
  singleInstruction = GDB::server.hasBreakpoints();
  endian = Context::Endian(ctx.endian);
  mode = Context::Mode(ctx.mode);
  cop1Enabled = cpu.scc.status.enable.coprocessor1 > 0;
  floatingPointMode = cpu.scc.status.floatingPointMode > 0;
  is64bit = ctx.bits == 64;
}

auto CPU::Context::JIT::toBits() const -> u32 {
  u32 bits = singleInstruction ? 1 << 6 : 0;
  bits |= endian ? 1 << 7 : 0;
  bits |= (mode & 0x03) << 9;
  bits |= cop1Enabled ? 1 << 10 : 0;
  bits |= floatingPointMode ? 1 << 11 : 0;
  bits |= is64bit ? 1 << 12 : 0;
  return bits;
}
