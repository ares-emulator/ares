auto Disc::IRQ::poll() -> void {
  bool pending = 0;
  pending |= ready.flag & ready.enable;
  pending |= complete.flag & complete.enable;
  pending |= acknowledge.flag & acknowledge.enable;
  pending |= end.flag & end.enable;
  pending |= error.flag & error.enable;
  interrupt.drive(Interrupt::CDROM, pending);
}
