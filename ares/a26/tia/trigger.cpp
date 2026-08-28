auto TIA::updateTriggerInput(n1 index) -> void {
  n1 value = index ? controllerPort2.read().bit(4) : controllerPort1.read().bit(4);
  triggers.sample(index, value);
}

auto TIA::readTrigger(n1 index) -> n1 {
  n1 value = index ? controllerPort2.read().bit(4) : controllerPort1.read().bit(4);
  return triggers.read(index, value);
}

auto TIA::TriggerInputs::sample(n1 index, n1 value) -> void {
  auto& input = this->input[index];
  if(input.mode) input.value &= value;
}

auto TIA::TriggerInputs::read(n1 index, n1 value) -> n1 {
  auto& input = this->input[index];
  sample(index, value);
  return input.mode ? input.value : value;
}

auto TIA::TriggerInputs::vblank(n1 latch) -> void {
  for(auto& input : this->input) {
    input.mode = latch;
    if(!input.mode) input.value = 1;
  }
}

auto TIA::TriggerInputs::power() -> void {
  //Stella powers the latch low; MAME and ares use high. Hardware is unknown.
  for(auto& input : this->input) input = {.mode = 0, .value = 1};
}
