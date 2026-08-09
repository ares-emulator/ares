#include <a52/a52.hpp>

namespace ares::Atari5200 {

POKEY pokey;

#include "clock.cpp"
#include "audio.cpp"
#include "dac.cpp"
#include "irq.cpp"
#include "pots.cpp"
#include "status.cpp"
#include "keyboard.cpp"
#include "serial.cpp"
#include "output.cpp"
#include "io.cpp"

auto POKEY::load(Node::Object parent) -> void {
  node = parent->append<Node::Object>("POKEY");
  stream = node->append<Node::Audio::Stream>("Audio");
  stream->setChannels(1);
  stream->setFrequency(system.frequency() / Timing::ColorClocksPerMachineCycle);
}

auto POKEY::unload() -> void {
  Thread::destroy();
  if(node && stream) node->remove(stream);
  stream.reset();
  node = {};
}

auto POKEY::main() -> void {
  audio.clockFilters();
  irq.clock();

  auto pulses = clock.clock(control & 3);
  auto timerEdges = audio.clockTimers(pulses, clock, serial.holdsTimer34());

  if(control.bit(2)) pots.clockFast();
  else if(pulses.clock15) pots.clock15();
  clockKeyboard(pulses.clock15);
  serial.clock(timerEdges, serialInput);
  setIRQ();

  clockOutput(output());
  step();
}

auto POKEY::step(u32 clocks) -> void {
  Thread::step(clocks);
  Thread::synchronize(cpu);
}

auto POKEY::power() -> void {
  Thread::create(system.frequency() / Timing::ColorClocksPerMachineCycle, std::bind_front(&POKEY::main, this));

  audio.power();
  clock.power();
  irq.power();
  pots.power();
  status.power();
  keyboard.power();
  serial.power();
  powerOutput();
  control = 0;
  setIRQ();
}

auto POKEY::sampleKeyboard(n6 scanAddress) -> KeyboardLines {
  auto& controller = controllerPorts[gtia.controllerSelect()];
  n4 code = scanAddress >> 1;
  return {
    .kr1 = code && controller.keypad(code),
    .kr2 = controller.topFire(),
  };
}

auto POKEY::clockKeyboard(bool clock15) -> void {
  if(!control.bit(1)) {
    keyboard.disable();
    return;
  }
  if(!clock15) return;
  keyboard.clock(control.bit(0));
}

}
