auto DMA::IRQ::poll() -> void {
  bool previous = flag;
  flag = force;
  if(enable) {
    flag |= self.channels[0].irq.flag;
    flag |= self.channels[1].irq.flag;
    flag |= self.channels[2].irq.flag;
    flag |= self.channels[3].irq.flag;
    flag |= self.channels[4].irq.flag;
    flag |= self.channels[5].irq.flag;
    flag |= self.channels[6].irq.flag;
  }
  if(!previous && flag) interrupt.raise(Interrupt::DMA);
  if(!flag) interrupt.lower(Interrupt::DMA);
}
