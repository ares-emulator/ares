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

  //todo: 64-bit mode breaks libdragon software, so disable it for now ...
  bits = 32;

  if(bits == 32) {
    segment[0] = Segment::Mapped32;
    segment[1] = Segment::Mapped32;
    segment[2] = Segment::Mapped32;
    segment[3] = Segment::Mapped32;
    switch(mode) {
    case Mode::Kernel:
      segment[4] = Segment::Cached;
      segment[5] = Segment::Uncached;
      segment[6] = Segment::Mapped32;
      segment[7] = Segment::Mapped32;
      break;
    case Mode::Supervisor:
      segment[4] = Segment::Invalid;
      segment[5] = Segment::Invalid;
      segment[6] = Segment::Mapped32;
      segment[7] = Segment::Invalid;
      break;
    case Mode::User:
      segment[4] = Segment::Invalid;
      segment[5] = Segment::Invalid;
      segment[6] = Segment::Invalid;
      segment[7] = Segment::Invalid;
      break;
    }
  }

  if(bits == 64) {
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
