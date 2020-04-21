//Sanyo LC7883KM

auto MCD::CDD::DAC::load(Node::Object parent) -> void {
  stream = parent->append<Node::Stream>("CD-DA");
  stream->setChannels(2);
  stream->setFrequency(44100);
}

auto MCD::CDD::DAC::unload() -> void {
  stream = {};
}

auto MCD::CDD::DAC::sample(int16 left, int16 right) -> void {
  left  = (left  * (int)attenuated) / 0x4000 >> 1;
  right = (right * (int)attenuated) / 0x4000 >> 1;

  stream->sample(left / 32768.0, right / 32768.0);

  if(attenuated == attenuator);
  else if(attenuated < attenuator) attenuated++;
  else if(attenuated > attenuator) attenuated--;
}

auto MCD::CDD::DAC::reconfigure() -> void {
  stream->resetFilters();
  double inputFrequency = 0.0;
  if(deemphasis == 0) return;  //no de-emphasis
  if(deemphasis == 1) inputFrequency = 44100.0;
  if(deemphasis == 2) inputFrequency = 32000.0;  //unverified behavior
  if(deemphasis == 3) inputFrequency = 48000.0;  //unverified behavior
  //todo: this should be a first-order filter, but nall/dsp lacks a first-order high-shelf filter
  stream->addHighShelfFilter((10000.0 + 3100.0) / 2.0, 2, -9.477, 0.5);
}

auto MCD::CDD::DAC::power(bool reset) -> void {
  rate = 0;
  deemphasis = 0;
  attenuator = 0x4000;
  attenuated = 0x4000;
}
