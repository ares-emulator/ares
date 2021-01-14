auto DMA::IRQ::poll() -> void {
  bool previous = flag;
  flag = force;
  if(enable) {
    flag |= self.channels[0].irq.flag & self.channels[0].irq.enable;
    flag |= self.channels[1].irq.flag & self.channels[1].irq.enable;
    flag |= self.channels[2].irq.flag & self.channels[2].irq.enable;
    flag |= self.channels[3].irq.flag & self.channels[3].irq.enable;
    flag |= self.channels[4].irq.flag & self.channels[4].irq.enable;
    flag |= self.channels[5].irq.flag & self.channels[5].irq.enable;
    flag |= self.channels[6].irq.flag & self.channels[6].irq.enable;
  }
  if(!previous && flag) interrupt.raise(Interrupt::DMA);
  if(!flag) interrupt.lower(Interrupt::DMA);
}
