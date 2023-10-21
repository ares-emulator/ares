auto VDP::PSG::load(Node::Object parent) -> void {
  node = parent->append<Node::Object>("PSG");

  stream = node->append<Node::Audio::Stream>("PSG");
  stream->setChannels(1);
  stream->setFrequency(system.frequency() / 15.0 / 16.0);
  stream->addHighPassFilter(  20.0, 1);
  stream->addLowPassFilter (2840.0, 1);
}

auto VDP::PSG::unload() -> void {
  node->remove(stream);
  stream.reset();
  node.reset();
}

auto VDP::PSG::main() -> void {
  auto channels = SN76489::clock();
  if(test.volumeOverride) {
    channels[0] = channels[test.volumeChannel];
    channels[1] = channels[test.volumeChannel];
    channels[2] = channels[test.volumeChannel];
    channels[3] = channels[test.volumeChannel];
  }

  double output = 0.0;
  output += volume[channels[0]];
  output += volume[channels[1]];
  output += volume[channels[2]];
  output += volume[channels[3]];
  stream->frame(output / 4.0 * 0.625);
  step(16);
}

auto VDP::PSG::step(u32 clocks) -> void {
  Thread::step(clocks);
  Thread::synchronize(cpu, apu);
}

auto VDP::PSG::power(bool reset) -> void {
  SN76489::power();
  Thread::create(system.frequency() / 15.0, {&PSG::main, this});

  test = {};

  for(u32 level : range(15)) {
    volume[level] = pow(2, level * -2.0 / 6.0);
  }
  volume[15] = 0;
}
