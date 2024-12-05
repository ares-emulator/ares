auto MI::serialize(serializer& s) -> void {
  s(ram);
  s(scratch);

  s(irq.sp.line);
  s(irq.sp.mask);
  s(irq.si.line);
  s(irq.si.mask);
  s(irq.ai.line);
  s(irq.ai.mask);
  s(irq.vi.line);
  s(irq.vi.mask);
  s(irq.pi.line);
  s(irq.pi.mask);
  s(irq.dp.line);
  s(irq.dp.mask);
  s(bb_irq.flash.line);
  s(bb_irq.flash.mask);
  s(bb_irq.aes.line);
  s(bb_irq.aes.mask);
  s(bb_irq.ide.line);
  s(bb_irq.ide.mask);
  s(bb_irq.pi_err.line);
  s(bb_irq.pi_err.mask);
  s(bb_irq.usb0.line);
  s(bb_irq.usb0.mask);
  s(bb_irq.usb1.line);
  s(bb_irq.usb1.mask);
  s(bb_irq.btn.line);
  s(bb_irq.btn.mask);
  s(bb_irq.md.line);
  s(bb_irq.md.mask);

  s(io.initializeLength);
  s(io.initializeMode);
  s(io.ebusTestMode);
  s(io.rdramRegisterSelect);

  s(revision.io);
  s(revision.rac);

  s(bb.button);
  s(bb.card);

  s(bb_trap.application);
  s(bb_trap.timer);
  s(bb_trap.pi_error);
  s(bb_trap.mi_error);
  s(bb_trap.button);
  s(bb_trap.md);

  s(bb_exc.secure);
  s(bb_exc.boot_swap);
  s(bb_exc.application);
  s(bb_exc.timer);
  s(bb_exc.pi_error);
  s(bb_exc.mi_error);
  s(bb_exc.button);
  s(bb_exc.md);
  s(bb_exc.sk_ram_access);
}
