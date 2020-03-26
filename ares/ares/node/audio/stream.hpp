struct Stream : Object {
  DeclareClass(Stream, "Stream")
  using Object::Object;

  inline auto channels() const -> uint { return _channels.size(); }
  inline auto frequency() const -> double { return _frequency; }
  inline auto resamplerFrequency() const -> double { return _resamplerFrequency; }

  auto setChannels(uint channels) -> void;
  auto setFrequency(double frequency) -> void;
  auto setResamplerFrequency(double resamplerFrequency) -> void;

  auto resetFilters() -> void;
  auto addLowPassFilter(double cutoffFrequency, uint order, uint passes = 1) -> void;
  auto addHighPassFilter(double cutoffFrequency, uint order, uint passes = 1) -> void;
  auto addLowShelfFilter(double cutoffFrequency, uint order, double gain, double slope) -> void;
  auto addHighShelfFilter(double cutoffFrequency, uint order, double gain, double slope) -> void;

  auto pending() const -> bool;
  auto read(double samples[]) -> uint;
  auto write(const double samples[]) -> void;

  template<typename... P>
  inline auto sample(P&&... p) -> void {
    if(runAhead()) return;
    double samples[sizeof...(p)] = {forward<P>(p)...};
    write(samples);
  }

protected:
  struct Filter {
    enum class Mode : uint { OnePole, Biquad } mode;
    enum class Type : uint { None, LowPass, HighPass, LowShelf, HighShelf } type;
    enum class Order : uint { None, First, Second } order;
    DSP::IIR::OnePole onePole;
    DSP::IIR::Biquad biquad;
  };
  struct Channel {
    vector<Filter> filters;
    vector<DSP::IIR::Biquad> nyquist;
    DSP::Resampler::Cubic resampler;
  };
  vector<Channel> _channels;
  double _frequency = 48000.0;
  double _resamplerFrequency = 48000.0;
};
