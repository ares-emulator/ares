inline auto ARM7TDMI::r(n4 index) -> GPR& {
  switch(index) {
  case  0: return processor.r0;
  case  1: return processor.r1;
  case  2: return processor.r2;
  case  3: return processor.r3;
  case  4: return processor.r4;
  case  5: return processor.r5;
  case  6: return processor.r6;
  case  7: return processor.r7;
  case  8: return processor.cpsr.m == PSR::FIQ ? processor.fiq.r8  : processor.r8;
  case  9: return processor.cpsr.m == PSR::FIQ ? processor.fiq.r9  : processor.r9;
  case 10: return processor.cpsr.m == PSR::FIQ ? processor.fiq.r10 : processor.r10;
  case 11: return processor.cpsr.m == PSR::FIQ ? processor.fiq.r11 : processor.r11;
  case 12: return processor.cpsr.m == PSR::FIQ ? processor.fiq.r12 : processor.r12;
  case 13: switch(processor.cpsr.m) {
    case PSR::USR: return processor.r13;
    case PSR::FIQ: return processor.fiq.r13;
    case PSR::IRQ: return processor.irq.r13;
    case PSR::SVC: return processor.svc.r13;
    case PSR::ABT: return processor.abt.r13;
    case PSR::UND: return processor.und.r13;
    case PSR::SYS: return processor.r13;
    default: return processor.rNULL;
  }
  case 14: switch(processor.cpsr.m) {
    case PSR::USR: return processor.r14;
    case PSR::FIQ: return processor.fiq.r14;
    case PSR::IRQ: return processor.irq.r14;
    case PSR::SVC: return processor.svc.r14;
    case PSR::ABT: return processor.abt.r14;
    case PSR::UND: return processor.und.r14;
    case PSR::SYS: return processor.r14;
    default: return processor.rNULL;
  }
  case 15: return processor.r15;
  }
  unreachable;
}

inline auto ARM7TDMI::cpsr() -> PSR& {
  return processor.cpsr;
}

inline auto ARM7TDMI::spsr() -> PSR& {
  switch(processor.cpsr.m) {
  case PSR::FIQ: return processor.fiq.spsr;
  case PSR::IRQ: return processor.irq.spsr;
  case PSR::SVC: return processor.svc.spsr;
  case PSR::ABT: return processor.abt.spsr;
  case PSR::UND: return processor.und.spsr;
  }
  return processor.spsrNULL;
}

inline auto ARM7TDMI::privileged() const -> bool {
  return processor.cpsr.m != PSR::USR;
}

inline auto ARM7TDMI::exception() const -> bool {
  return privileged() && processor.cpsr.m != PSR::SYS;
}
