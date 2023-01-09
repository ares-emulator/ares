auto CPU::read(n16 address) -> n8 {
  if (address < 0x4000) {
    if (expansionPort.connected() && expansionPort.romcs()) {
      return expansionPort.read(address);
    }

    if (system.romBank) {
      return rom.sub.read(address);
    }

    return rom.bios.read(address);
  }

  if (Model::Spectrum48k()) {
    return ram.read(address - 0x4000);
  }

  if (address < 0x8000) {
    return readBanked(5, address - 0x4000);
  }

  if (address < 0xC000) {
    return readBanked(2, address - 0x8000);
  }

  return readBanked(system.ramBank, address - 0xC000);
}

auto CPU::write(n16 address, n8 data) -> void {
  if (expansionPort.connected() && expansionPort.mapped(address, false)) {
    expansionPort.write(address, data);
  }

  if (address < 0x4000) {
    return;
  }

  if (Model::Spectrum48k()) {
    return ram.write(address - 0x4000, data);
  }

  if (address < 0x8000) {
    return writeBanked(5, address - 0x4000, data);
  }

  if (address < 0xC000) {
    return writeBanked(2, address - 0x8000, data);
  }

  return writeBanked(system.ramBank, address - 0xC000, data);
}

auto CPU::readBanked(n3 bank, n16 address) -> n8 {
  return ram.read((0x4000 * (n32)bank) + address);
}

auto CPU::writeBanked(n3 bank, n16 address, n8 data) -> void {
  return ram.write((0x4000 * (n32)bank) + address, data);
}

auto CPU::in(n16 address) -> n8 {
  if (expansionPort.connected() && expansionPort.mapped(address, true)) {
    if (address.bit(0) == 0) {
      // Some expansion port devices override keyboard (bits 0-4) of the ULA
      n8 value = ula.in(address);
      n8 expValue = expansionPort.in(address);
      value.bit(0, 4) &= expValue.bit(0,4);
      return value;
    }

    return expansionPort.in(address);
  }

  if (address.bit(0) == 0) {
    return ula.in(address);
  }

  if (Model::Spectrum128()) {
    if (address.bit(1) == 0 && address.bit(15) == 1) {
      return psg.read();
    }
  }

  return ula.floatingBus();
}

auto CPU::out(n16 address, n8 data) -> void {
  if (expansionPort.connected() && expansionPort.mapped(address, true)) {
    expansionPort.out(address, data);
  }

  if (address.bit(0) == 0) {
    ula.out(data);
  }

  if (Model::Spectrum128()) {
    if (address.bit(1) == 0 && address.bit(15) == 0 && !system.pagingDisabled) {
      system.ramBank = data.bit(0, 2);
      system.screenBank = data.bit(3);
      system.romBank = data.bit(4);
      system.pagingDisabled = data.bit(5);
    }

    if (address.bit(1) == 0 && address.bit(15) == 1) {
      if (address.bit(14) == 1) {
        psg.select(data);
      }

      if (address.bit(14) == 0) {
        psg.write(data);
      }
    }
  }
}
