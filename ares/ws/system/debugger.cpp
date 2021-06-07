auto System::Debugger::load(Node::Object parent) -> void {
  memory.eeprom = parent->append<Node::Debugger::Memory>("System EEPROM");
  memory.eeprom->setSize(self.eeprom.size);
  memory.eeprom->setRead([&](u32 address) -> u8 {
    return self.eeprom.data[address];
  });
  memory.eeprom->setWrite([&](u32 address, u8 data) -> void {
    self.eeprom.data[address] = data;
  });
}

auto System::Debugger::unload(Node::Object parent) -> void {
  parent->remove(memory.eeprom);
  memory.eeprom.reset();
}
