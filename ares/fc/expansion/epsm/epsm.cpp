EPSM::EPSM(Node::Port parent) : ymf288(interface) {
  node = parent->append<Node::Peripheral>("EPSM");
  ioAddress = 0x401c;

  streamFM = node->append<Node::Audio::Stream>("EPSM FM");
  streamFM->setChannels(2);
  streamFM->setFrequency(ymf288.sample_rate(8_MHz));
  streamFM->addHighPassFilter(  20.0, 1);
  streamFM->addLowPassFilter(2840.0, 1);

  streamSSG = node->append<Node::Audio::Stream>("EPSM SSG");
  streamSSG->setChannels(1);
  streamSSG->setFrequency(ymf288.sample_rate(8_MHz));

  ymf288.reset();
  clocksPerSample = clocksPerSample = 8_MHz / ymf288.sample_rate(8_MHz);
  Thread::create(8_MHz, std::bind_front(&EPSM::main, this));
}

EPSM::~EPSM() {
  node->remove(streamFM);
  node->remove(streamSSG);
  node.reset();
  Thread::destroy();
}

auto EPSM::main() -> void {
  ymfm::ymf288::output_data output;
  ymf288.generate(&output);

  streamFM->frame(output.data[0] / 32768.0, output.data[1] / 32768.0);
  streamSSG->frame(output.data[2] / (4.0 * 32768.0));

  step(clocksPerSample);
}

auto EPSM::step(u32 clocks) -> void {
  if(busyCyclesRemaining) {
    busyCyclesRemaining -= clocks;
    if(busyCyclesRemaining <= 0) {
      busyCyclesRemaining = 0;
    }
  }

  for(u32 timer : range(2)) {
    if(timerCyclesRemaining[timer]) {
      timerCyclesRemaining[timer] -= clocks;
      if(timerCyclesRemaining[timer] <= 0) {
        timerCyclesRemaining[timer] = 0;
        interface.timerCallback(timer);
      }
    }
  }

  Thread::step(clocks);
  Thread::synchronize();
}

auto EPSM::read1() -> n1 {
  return 0;
}

auto EPSM::read2() -> n5 {
  return 0b00000;
}

auto EPSM::write(n8 data) -> void {
  if (data.bit(1) != latch) {
    latch = data.bit(1);
    if (latch) {
      ymAddress.bit(0,1) = data.bit(2,3);
      ymData.bit(4,7) = data.bit(4,7);
    } else {
      ymData.bit(0,3) = data.bit(4,7);
      ymf288.write(ymAddress, ymData);
    }
  }
}

auto EPSM::writeIO(n16 address, n8 data) -> void {
  if ((address & 0xFFFC) != ioAddress) return;
  ymf288.write(address, data);
}

void EPSM::Interface::ymfm_update_irq(bool asserted) {
  // TODO: Handle conflicts between cartridge and EPSM IRQ.
  cpu.irqLine(asserted ? 1 : 0);
}
