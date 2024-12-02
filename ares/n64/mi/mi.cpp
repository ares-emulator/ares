#include <n64/n64.hpp>

namespace ares::Nintendo64 {

MI mi;
#include "io.cpp"
#include "debugger.cpp"
#include "serialization.cpp"

auto MI::load(Node::Object parent) -> void {
  node = parent->append<Node::Object>("MI");
  if (system._BB()) {
    rom.allocate(0x2000);
    ram.allocate(0x10000);
    scratch.allocate(0x8000);
  }

  debugger.load(node);
}

auto MI::unload() -> void {
  node.reset();
  if (system._BB()) {
    rom.reset();
    ram.reset();
  }
  debugger = {};
}

auto MI::raise(IRQ source) -> void {
  debugger.interrupt((u32)source);
  switch(source) {
  case IRQ::SP:     irq.sp.line = 1; break;
  case IRQ::SI:     irq.si.line = 1; break;
  case IRQ::AI:     irq.ai.line = 1; break;
  case IRQ::VI:     irq.vi.line = 1; break;
  case IRQ::PI:     irq.pi.line = 1; break;
  case IRQ::DP:     irq.dp.line = 1; break;
  case IRQ::FLASH:  bb_irq.flash .line = 1; break;
  case IRQ::AES:    bb_irq.aes   .line = 1; break;
  case IRQ::IDE:    bb_irq.ide   .line = 1; break;
  case IRQ::PI_ERR: bb_irq.pi_err.line = 1; break;
  case IRQ::USB0:   bb_irq.usb0  .line = 1; break;
  case IRQ::USB1:   bb_irq.usb1  .line = 1; break;
  case IRQ::BTN:    bb_irq.btn   .line = 1; break;
  case IRQ::MD:     bb_irq.md    .line = 1; break;
  }
  poll();
}

auto MI::lower(IRQ source) -> void {
  switch(source) {
  case IRQ::SP:     irq.sp.line = 0; break;
  case IRQ::SI:     irq.si .line = 0; break;
  case IRQ::AI:     irq.ai.line = 0; break;
  case IRQ::VI:     irq.vi.line = 0; break;
  case IRQ::PI:     irq.pi.line = 0; break;
  case IRQ::DP:     irq.dp.line = 0; break;
  case IRQ::FLASH:  bb_irq.flash .line = 0; break;
  case IRQ::AES:    bb_irq.aes   .line = 0; break;
  case IRQ::IDE:    bb_irq.ide   .line = 0; break;
  case IRQ::PI_ERR: bb_irq.pi_err.line = 0; break;
  case IRQ::USB0:   bb_irq.usb0  .line = 0; break;
  case IRQ::USB1:   bb_irq.usb1  .line = 0; break;
  case IRQ::BTN:    bb_irq.btn   .line = 0; break;
  case IRQ::MD:     bb_irq.md    .line = 0; break;
  }
  poll();
}

auto MI::poll() -> void {
  bool line = 0;
  line |= irq.sp.line & irq.sp.mask;
  line |= irq.si.line & irq.si.mask;
  line |= irq.ai.line & irq.ai.mask;
  line |= irq.vi.line & irq.vi.mask;
  line |= irq.pi.line & irq.pi.mask;
  line |= irq.dp.line & irq.dp.mask;
  if(system._BB()) {
    line |= bb_irq.flash .line & bb_irq.flash .mask;
    line |= bb_irq.aes   .line & bb_irq.aes   .mask;
    line |= bb_irq.ide   .line & bb_irq.ide   .mask;
    line |= bb_irq.pi_err.line & bb_irq.pi_err.mask;
    line |= bb_irq.usb0  .line & bb_irq.usb0  .mask;
    line |= bb_irq.usb1  .line & bb_irq.usb1  .mask;
    line |= bb_irq.btn   .line & bb_irq.btn   .mask;
    line |= bb_irq.md    .line & bb_irq.md    .mask;

    cpu.scc.nmiPending |= enter_secure_mode();
  }
  cpu.scc.cause.interruptPending.bit(2) = line;
}

auto MI::enter_secure_mode() const -> bool {
  bool enter = 0;
  enter |= bb_exc.application;
  enter |= bb_exc.timer;
  enter |= bb_exc.pi_error;
  enter |= bb_exc.mi_error;
  enter |= bb_exc.button;
  enter |= bb_exc.md;
  return enter;
}

auto MI::power(bool reset) -> void {
  irq = {};
  io = {};

  if (system._BB()) {
    revision.io = 0xB0;
    revision.rac = 0xB0;
    if(auto fp = system.pak->read("boot.rom")) {
      rom.load(fp);
    }
    ram.fill();
    scratch.fill();
    bb = {};
    bb_exc = {};
    bb_exc.boot_swap = 1;
  }

}

}
