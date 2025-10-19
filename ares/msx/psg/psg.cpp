#include <msx/msx.hpp>

namespace ares::MSX {

PSG psg;
#include "serialization.cpp"

auto PSG::load(Node::Object parent) -> void {
  node = parent->append<Node::Object>("PSG");

  stream = node->append<Node::Audio::Stream>("PSG");
  stream->setChannels(1);
  stream->setFrequency(system.colorburst() / 16.0);
  stream->addHighPassFilter(20.0, 1);
}

auto PSG::unload() -> void {
  node->remove(stream);
  stream.reset();
  node.reset();
}

auto PSG::main() -> void {
  auto channels = AY38910::clock();
  double output = 0.0;
  output += volume[channels[0]];
  output += volume[channels[1]];
  output += volume[channels[2]];
  stream->frame(output / 3.0);
  step(1);
}

auto PSG::step(u32 clocks) -> void {
  Thread::step(clocks);
  Thread::synchronize(cpu);
}

auto PSG::power() -> void {
  AY38910::power();
  Thread::create(system.colorburst() / 16.0, std::bind_front(&PSG::main, this));

  for(u32 level : range(16)) {
    volume[level] = 1.0 / pow(2, 1.0 / 2 * (15 - level));
  }
}

auto PSG::readIO(n1 port) -> n8 {
  // port 0 refers to PSG register 14, which receives inputs from
  // either joystick depending on the state of Bit 6 in PSG register
  // 15.
  if (port == 0) {
    n8 data = controllerMux.read();
    data.bit(7) = tapeDeck.read();
    return data;
  }

  // port 1 refers to PSG register 15, normally used for output.
  // If read from, return the last value written.
  // TODO: What does this do on a real MSX?
  if (port == 1) return portB.data;

  unreachable;
}

auto PSG::writeIO(n1 port, n8 data) -> void {
  n8 out_port1, out_port2;

  // port 0 refers to PSG register 14, typically only used as an input
  if(port == 0) return;

  // port 1 refers to PSG register 15, with bits controlling level of
  // outputs going to both ports.

  out_port1.bit(0,1) = data.bit(0,1); // Pin 6,7 of general port 1
  out_port1.bit(2)   = data.bit(4);   // Pin 8 of general port 1

  out_port2.bit(0,1) = data.bit(2,3); // Pin 6,7 of general port 2
  out_port2.bit(2)   = data.bit(5);   // Pin 8 of general port 2

  controllerPort1.write(out_port1);
  controllerPort2.write(out_port2);

  // This bit controls which port is read through PSG register 14
  controllerMux.select(data.bit(6));
}

}
