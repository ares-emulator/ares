auto Cartridge::MBC7::EEPROM::load(Markup::Node document) -> void {
  if(auto memory = document["game/board/memory(type=EEPROM,content=Save)"]) {
    uint size  = memory["size"].natural();
    uint width = memory["width"].natural();
    allocate(size, width);

    if(auto fp = platform->open(cartridge.node, "save.eeprom", File::Read, File::Optional)) {
      fp->read(data, min(fp->size(), sizeof(data)));
    }
  }
}

auto Cartridge::MBC7::EEPROM::save(Markup::Node document) -> void {
  if(auto memory = document["game/board/memory(type=EEPROM,content=Save)"]) {
    if(auto fp = platform->open(cartridge.node, "save.eeprom", File::Write)) {
      fp->write(data, size);
    }
  }
}

auto Cartridge::MBC7::EEPROM::power() -> void {
  M93LCx6::power();
  select = 0;
  clock = 0;
}

auto Cartridge::MBC7::EEPROM::main() -> void {
  M93LCx6::clock();

  //step by approximately one millisecond
  cartridge.step(cartridge.frequency() / 1000);
}

auto Cartridge::MBC7::EEPROM::readIO() -> uint8 {
  uint8 data = 0b00'1111'00;
  if(!select) {
    data.bit(0) = 1;  //high-z when the chip is idle (not selected)
  } else if(busy) {
    data.bit(0) = 0;  //low when a programming command is in progress
  } else if(output.count) {
    data.bit(0) = output.edge();  //shift register data during read commands
  } else {
    data.bit(0) = 1;  //high-z during all other commands
  }
  data.bit(1) = input.edge();
  data.bit(6) = clock;
  data.bit(7) = select;
  return data;
}

auto Cartridge::MBC7::EEPROM::writeIO(uint8 data) -> void {
  //chip enters idle state on falling CS edge
  if(select && !data.bit(7)) return power();

  //chip leaves idle state on rising CS edge
  if(!(select = data.bit(7))) return;

  //input shift register clocks on rising edge
  if(!clock.raise(data.bit(6))) return;

  //read mode
  if(output.count && !data.bit(1)) {
    if(input.start() && *input.start() == 1) {
      if(input.opcode() && *input.opcode() == 0b10) {
        output.read();
        if(output.count == 0) {
          //sequential read mode
          input.increment();
          read();
        }
      }
    }
    return;
  }
  output.flush();

  input.write(data.bit(1));
  edge();
}
