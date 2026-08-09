auto POKEY::powerOutput() -> void {
  auto frequency = system.frequency() / Timing::ColorClocksPerMachineCycle;
  outputDecay = exp(-1.0 / (frequency * OutputTimeConstant));
  previousInput = 0.0;
  coupledOutput = 0.0;
}

auto POKEY::clockOutput(f64 input) -> void {
  coupledOutput = coupledOutput * outputDecay + input - previousInput;
  previousInput = input;
  stream->frame(coupledOutput);
}
