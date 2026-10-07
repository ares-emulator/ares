auto Disc::IRQ::poll() -> void {
  interrupt.drive(Interrupt::CDROM, (flag & mask) != 0);
}

auto Disc::IRQ::pending() -> bool {
  return flag != 0;
}
