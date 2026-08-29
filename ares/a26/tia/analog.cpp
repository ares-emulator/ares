auto TIA::AnalogInputs::power(f64 frequency) -> void {
  for(auto& input : this->input) input = {};
  time = 0;
  dumped = 0;
  this->frequency = frequency;

  tripVoltage = SupplyVoltage * (1.0 - std::exp(
    -TripClocks / frequency / (SeriesResistance + CalibrationResistance) / Capacitance
  ));
}

auto TIA::AnalogInputs::advance() -> void {
  time++;
}

auto TIA::AnalogInputs::update(n2 index, Controller::AnalogConnection connection) -> void {
  auto& input = this->input[index];
  if(input.connection.type == connection.type && input.connection.resistance == connection.resistance) return;
  advance(input);
  input.connection = connection;
}

auto TIA::AnalogInputs::vblank(n1 dumped) -> void {
  for(auto& input : this->input) advance(input);
  this->dumped = dumped;
}

auto TIA::AnalogInputs::read(n2 index) -> n1 {
  advance(input[index]);
  return !dumped && input[index].voltage > tripVoltage;
}

auto TIA::AnalogInputs::advance(Input& input) -> void {
  auto clocks = time - input.timestamp;
  input.timestamp = time;
  if(!clocks) return;

  auto seconds = clocks / frequency;

  if(dumped) {
    input.voltage *= std::exp(-seconds / DumpResistance / Capacitance);
    return;
  }
  using Type = Controller::AnalogConnection::Type;
  if(input.connection.type == Type::Disconnected) return;
  auto decay = std::exp(-seconds / (SeriesResistance + input.connection.resistance) / Capacitance);
  auto target = input.connection.type == Type::Vcc ? SupplyVoltage : 0.0;
  input.voltage = target + (input.voltage - target) * decay;
}

auto TIA::updateAnalogInput(n2 index) -> void {
  auto& port = index < 2 ? controllerPort1 : controllerPort2;
  analog.update(index, port.readAnalog(index.bit(0)));
}
