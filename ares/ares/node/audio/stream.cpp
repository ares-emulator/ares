auto Stream::setChannels(uint channels) -> void {
  _channels.reset();
  _channels.resize(channels);
}

auto Stream::setFrequency(double frequency) -> void {
  _frequency = frequency;
  setResamplerFrequency(_resamplerFrequency);
}

auto Stream::setResamplerFrequency(double resamplerFrequency) -> void {
  _resamplerFrequency = resamplerFrequency;

  for(auto& channel : _channels) {
    channel.nyquist.reset();
    channel.resampler.reset(_frequency, _resamplerFrequency);
  }

  if(_frequency >= _resamplerFrequency * 2) {
    //add a low-pass filter to prevent aliasing during resampling
    double cutoffFrequency = min(25000.0, _resamplerFrequency / 2.0 - 2000.0);
    for(auto& channel : _channels) {
      uint passes = 3;
      for(uint pass : range(passes)) {
        DSP::IIR::Biquad filter;
        double q = DSP::IIR::Biquad::butterworth(passes * 2, pass);
        filter.reset(DSP::IIR::Biquad::Type::LowPass, cutoffFrequency, _frequency, q);
        channel.nyquist.append(filter);
      }
    }
  }
}

auto Stream::setMuted(bool muted) -> void {
  _muted = muted;
}

auto Stream::resetFilters() -> void {
  for(auto& channel : _channels) {
    channel.filters.reset();
  }
}

auto Stream::addLowPassFilter(double cutoffFrequency, uint order, uint passes) -> void {
  for(auto& channel : _channels) {
    for(uint pass : range(passes)) {
      if(order == 1) {
        Filter filter{Filter::Mode::OnePole, Filter::Type::LowPass, Filter::Order::First};
        filter.onePole.reset(DSP::IIR::OnePole::Type::LowPass, cutoffFrequency, _frequency);
        channel.filters.append(filter);
      }
      if(order == 2) {
        Filter filter{Filter::Mode::Biquad, Filter::Type::LowPass, Filter::Order::Second};
        double q = DSP::IIR::Biquad::butterworth(passes * 2, pass);
        filter.biquad.reset(DSP::IIR::Biquad::Type::LowPass, cutoffFrequency, _frequency, q);
        channel.filters.append(filter);
      }
    }
  }
}

auto Stream::addHighPassFilter(double cutoffFrequency, uint order, uint passes) -> void {
  for(auto& channel : _channels) {
    for(uint pass : range(passes)) {
      if(order == 1) {
        Filter filter{Filter::Mode::OnePole, Filter::Type::HighPass, Filter::Order::First};
        filter.onePole.reset(DSP::IIR::OnePole::Type::HighPass, cutoffFrequency, _frequency);
        channel.filters.append(filter);
      }
      if(order == 2) {
        Filter filter{Filter::Mode::Biquad, Filter::Type::HighPass, Filter::Order::Second};
        double q = DSP::IIR::Biquad::butterworth(passes * 2, pass);
        filter.biquad.reset(DSP::IIR::Biquad::Type::HighPass, cutoffFrequency, _frequency, q);
        channel.filters.append(filter);
      }
    }
  }
}

auto Stream::addLowShelfFilter(double cutoffFrequency, uint order, double gain, double slope) -> void {
  for(auto& channel : _channels) {
    if(order == 2) {
      Filter filter{Filter::Mode::Biquad, Filter::Type::LowShelf, Filter::Order::Second};
      double q = DSP::IIR::Biquad::shelf(gain, slope);
      filter.biquad.reset(DSP::IIR::Biquad::Type::LowShelf, cutoffFrequency, _frequency, q);
      channel.filters.append(filter);
    }
  }
}

auto Stream::addHighShelfFilter(double cutoffFrequency, uint order, double gain, double slope) -> void {
  for(auto& channel : _channels) {
    if(order == 2) {
      Filter filter{Filter::Mode::Biquad, Filter::Type::HighShelf, Filter::Order::Second};
      double q = DSP::IIR::Biquad::shelf(gain, slope);
      filter.biquad.reset(DSP::IIR::Biquad::Type::HighShelf, cutoffFrequency, _frequency, q);
      channel.filters.append(filter);
    }
  }
}

auto Stream::pending() const -> bool {
  return _channels && _channels[0].resampler.pending();
}

auto Stream::read(double samples[]) -> uint {
  for(uint c : range(_channels.size())) samples[c] = _channels[c].resampler.read() * !muted();
  return _channels.size();
}

auto Stream::write(const double samples[]) -> void {
  for(uint c : range(_channels.size())) {
    double sample = samples[c] + 1e-25;  //constant offset used to suppress denormals
    for(auto& filter : _channels[c].filters) {
      switch(filter.mode) {
      case Filter::Mode::OnePole: sample = filter.onePole.process(sample); break;
      case Filter::Mode::Biquad: sample = filter.biquad.process(sample); break;
      }
    }
    for(auto& filter : _channels[c].nyquist) {
      sample = filter.process(sample);
    }
    _channels[c].resampler.write(sample);
  }

  //if there are samples pending, then alert the frontend to possibly process them.
  //this will generally happen when every audio stream has pending samples to be mixed.
  if(pending()) platform->audio(shared());
}
