auto DMA::IRQ::poll() -> void {
  flag = force;
  if(enable) {
    flag |= self.channel[0].irq.flag & self.channel[0].irq.enable;
    flag |= self.channel[1].irq.flag & self.channel[1].irq.enable;
    flag |= self.channel[2].irq.flag & self.channel[2].irq.enable;
    flag |= self.channel[3].irq.flag & self.channel[3].irq.enable;
    flag |= self.channel[4].irq.flag & self.channel[4].irq.enable;
    flag |= self.channel[5].irq.flag & self.channel[5].irq.enable;
    flag |= self.channel[6].irq.flag & self.channel[6].irq.enable;
  }
  interrupt.drive(Interrupt::DMA, flag);
}
