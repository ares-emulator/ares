auto CPU::IRQ::poll() -> void {
  pending = 0;
  if(cpu.r.p.i) return;

  if(!disable.irq2 && pcd.irqLine()) {
    pending = Vector::IRQ2;
  }

  if(!disable.irq1 && (vdp.vdc0.irqLine() | vdp.vdc1.irqLine())) {
    pending = Vector::IRQ1;
  }

  if(!disable.tiq && cpu.timer.irqLine()) {
    pending = Vector::TIQ;
  }
}
