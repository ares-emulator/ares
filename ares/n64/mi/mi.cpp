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

inline auto MI::stepBBTimer() -> bool {
  // Tick rate down at the RCP clock rate
  if((bb_timer.rate == 0) || (bb_timer.written))
    bb_timer.rate = bb_timer.rateStore;
  else if(bb_timer.rateStore != 0)
    bb_timer.rate -= 1;

  // On rate underflow, reload rate to rateStore and decr count

  b1 do_tick = (bb_timer.rateStore != 0) && (bb_timer.rate == 0);
  b1 underflow = do_tick && (bb_timer.count == 0);
  if(underflow || bb_timer.written)
    bb_timer.count = bb_timer.countStore;
  else if(do_tick)
    bb_timer.count -= 1;

  // When count runs out trigger NMI if we aren't in secure mode

  bb_timer.written = 0;

  return (!secure() && underflow);
}

inline auto MI::stepBBButtonTimer() -> bool {
  auto cont = (Gamepad*)controllerPort1.device.data();

  if(cont)
    bb.button = cont->bb_button->value();

  bb_button_timer.enable = (bb_button_timer.enable | bb.button) & bb_irq.btn.mask;

  cpu.scc.cause.interruptPending.bit(CPU::Interrupt::Reset) = bb_button_timer.enable;

  if(!bb_exc.enable_button || (bb_button_timer.count >= 0x100000))
    bb_button_timer.count = 0;

  bb_button_timer.div += 1;

  if(bb_button_timer.div == 0) {
    if(cont)
      platform->input(cont->bb_button);

    if(bb_button_timer.enable)
      bb_button_timer.count += 1;
  }

  return (bb_button_timer.count >= 0x100000);
}

auto MI::main() -> void {
  if (!system._BB()) return;

  bool trap = false;

  while((Thread::clock < 0) && !trap) {
    step(1 * 3);
    if(stepBBTimer()) {
      bb_trap.timer = 1;
      trap = true;
    }
    if(stepBBButtonTimer()) {
      bb_trap.button = 1;
      trap = true;
    }
  }

  if(trap)
    poll();
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
  cpu.scc.cause.interruptPending.bit(CPU::Interrupt::RCP) = line;

  if(system._BB()) {
    bool line = 0;
    line |= bb_irq.flash .line & bb_irq.flash .mask;
    line |= bb_irq.aes   .line & bb_irq.aes   .mask;
    line |= bb_irq.ide   .line & bb_irq.ide   .mask;
    line |= bb_irq.pi_err.line & bb_irq.pi_err.mask;
    line |= bb_irq.usb0  .line & bb_irq.usb0  .mask;
    line |= bb_irq.usb1  .line & bb_irq.usb1  .mask;
    line |= bb_irq.btn   .line & bb_irq.btn   .mask;
    line |= bb_irq.md    .line & bb_irq.md    .mask;

    cpu.scc.cause.interruptPending.bit(CPU::Interrupt::BB) = line;

    bb_exc.application |= bb_trap.application;
    bb_exc.timer       |= bb_trap.timer;
    bb_exc.pi_error    |= bb_trap.pi_error;
    bb_exc.mi_error    |= bb_trap.mi_error;
    bb_exc.button      |= bb_trap.button;
    bb_exc.md          |= bb_trap.md;

    //this isn't right, but the behaviour is the same
    bb_trap.application = 0;
    bb_trap.timer = 0;
    bb_trap.button = 0;

    if (enter_secure_mode() && !secure()) {
      if constexpr(Accuracy::CPU::Recompiler) {
        cpu.recompiler.invalidateRange(0x1fc0'0000, 0x80000);
      }
      cpu.scc.nmiStrobe |= 1;
      cpu.pipeline.exception();
    }
  }
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
    bb_timer = {};
    bb_button_timer = {};
    bb_exc = {};
    bb_exc.boot_swap = 1;
  }

}

}
