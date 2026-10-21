auto Disc::IRQ::poll() -> void {
  u8 flags = error.flag ? 5 : end.flag ? 4 : acknowledge.flag ? 3 : complete.flag ? 2 : ready.flag ? 1 : 0;
  u8 mask = ready.enable | complete.enable << 1 | acknowledge.enable << 2;
  interrupt.drive(Interrupt::CDROM, (flags & mask) != 0);
}

auto Disc::IRQ::pending() -> bool {
  bool pending = 0;
  pending |= ready.flag;
  pending |= complete.flag;
  pending |= acknowledge.flag;
  pending |= end.flag;
  pending |= error.flag;
  return pending;
}
