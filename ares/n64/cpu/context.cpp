auto CPU::Context::setMode() -> void {
  mode = min(2, self.scc.status.privilegeMode);
  if(self.scc.status.exceptionLevel) mode = Mode::Kernel;
  if(self.scc.status.errorLevel) mode = Mode::Kernel;

  segment[0] = Segment::Mapped;
  segment[1] = Segment::Mapped;
  segment[2] = Segment::Mapped;
  segment[3] = Segment::Mapped;
  if(mode == Mode::Kernel) {
    segment[4] = Segment::Cached;
    segment[5] = Segment::Uncached;
    segment[6] = Segment::Mapped;
    segment[7] = Segment::Mapped;
  }
  if(mode == Mode::Supervisor) {
    segment[4] = Segment::Invalid;
    segment[5] = Segment::Invalid;
    segment[6] = Segment::Mapped;
    segment[7] = Segment::Invalid;
  }
  if(mode == Mode::User) {
    segment[4] = Segment::Invalid;
    segment[5] = Segment::Invalid;
    segment[6] = Segment::Invalid;
    segment[7] = Segment::Invalid;
  }
}
