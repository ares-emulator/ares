auto System::Debugger::load(Node::Object parent) -> void {
  memory.eeprom = parent->append<Node::Debugger::Memory>("System EEPROM");
  memory.eeprom->setSize(self.eeprom.size);
  memory.eeprom->setRead([&](u32 address) -> u8 {
    return self.eeprom.data[address];
  });
  memory.eeprom->setWrite([&](u32 address, u8 data) -> void {
    self.eeprom.data[address] = data;
  });

  properties.ports = parent->append<Node::Debugger::Properties>("SoC I/O");
  properties.ports->setQuery([&] { return ports(); });
}

auto System::Debugger::unload(Node::Object parent) -> void {
  parent->remove(properties.ports);
  parent->remove(memory.eeprom);
  properties.ports.reset();
  memory.eeprom.reset();
}

auto System::Debugger::ports() -> string {
  string output;

  output.append("System Mode: ");
  if(system.mode().bit(0,2) >= 7) { output.append("Color, 4bpp packed"); }
  else if(system.mode().bit(0,2) >= 6) { output.append("Color, 4bpp planar"); }
  else if(system.mode().bit(0,2) >= 4) { output.append("Color, 2bpp planar"); }
  else output.append("Mono");
  output.append("\n");

  output.append(cpu.debugger.ports());

  return output;
}
