auto CPU::read(n1 upper, n1 lower, n24 address, n16 data) -> n16 {
  while(bus.acquired()) wait(1);
  return bus.read(upper, lower, address, data);
}

auto CPU::write(n1 upper, n1 lower, n24 address, n16 data) -> void {
  while(bus.acquired()) wait(1);
  return bus.write(upper, lower, address, data);
}
