auto Disc::IRQ::poll() -> void {
  bool pending = 0;
  pending |= ready.flag & ready.enable;
  pending |= complete.flag & complete.enable;
  pending |= acknowledge.flag & acknowledge.enable;
  pending |= end.flag & end.enable;
  pending |= error.flag & error.enable;

  if(pending == 0) cpu.interrupt.lower(CPU::Interrupt::CDROM);
  if(pending == 1) cpu.interrupt.raise(CPU::Interrupt::CDROM), print("* IRQ\n");
}
