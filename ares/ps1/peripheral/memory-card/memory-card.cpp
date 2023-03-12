MemoryCard::MemoryCard(Node::Port parent) {
  node = parent->append<Node::Peripheral>("Memory Card");
  node->setPak(pak = platform->pak(node));

  memory.allocate(128_KiB);
  format();

  if(auto fp = pak->read("save.card")) {
    if(fp->attribute("loaded").boolean()) {
      memory.load(fp);
    }
  }

  flag.value = 0x00;
  flag.error = 0;
  flag.fresh = 1;
  flag.unknown = 1;
}

MemoryCard::~MemoryCard() {
}

auto MemoryCard::save() -> void {
  if(auto fp = pak->write("save.card")) {
    memory.save(fp);
  }
}

auto MemoryCard::reset() -> void {
  state = State::Idle;
}

auto MemoryCard::acknowledge() -> bool {
  return state != State::Idle;
}

auto MemoryCard::bus(u8 data) -> u8 {
  n8 input  = data;
  n8 output = 0xff;

  if(state == State::Idle) {
    command = Command::None;
  }

  switch(command) {
  case Command::Identify: return identify(input);
  case Command::Read: return read(input);
  case Command::Write: return write(input);
  }

  switch(state) {

  case State::Idle: {
    if(input != 0x81) break;
    output = 0xff;
    state = State::Select;
    break;
  }

  case State::Select: {
    if(input == 'S') phase = 2, command = Command::Identify;
    if(input == 'R') phase = 2, command = Command::Read;
    if(input == 'W') phase = 2, command = Command::Write;

    output = flag.value;
    flag.error = 0;

    if(command == Command::None) {
      state = State::Idle;
    }
    break;
  }

  }

  return output;
}

auto MemoryCard::identify(u8 data) -> u8 {
  n8 input  = data;
  n8 output = 0xff;
  command = Command::None;
  return output;
}

auto MemoryCard::read(u8 data) -> u8 {
  n8 input  = data;
  n8 output = 0xff;

  switch(phase++) {
  case   2: return 0x5a;  //ID lower
  case   3: return 0x5d;  //ID upper
  case   4: address = address & 0x00ff | input << 8; return 0;
  case   5: address = address & 0xff00 | input << 0; return 0;
  case   6: return 0x5c;  //ACK lower
  case   7: return 0x5d;  //ACK upper
  case   8: checksum  = address >> 8; return address >> 8;
  case   9: checksum ^= address >> 0; return address >> 0;
  case range128(10, 137):
    output = memory.readByte(address * 128 + (phase - 11));
    checksum ^= output;
    return output;
  case 138: return checksum;
  case 139: state = State::Idle; command = Command::None; return 'G';
  }

  return output;
}

auto MemoryCard::write(u8 data) -> u8 {
  n8 input  = data;
  n8 output = 0xff;

  switch(phase++) {
  case   2: return 0x5a;  //ID lower
  case   3: return 0x5d;  //ID upper
  case   4:
    address = address & 0x00ff | input << 8;
    checksum  = address >> 8;
    return 0;
  case   5:
    address = address & 0xff00 | input << 0;
    checksum ^= address >> 0;
    response = 'G';
    if(address >= 1024) {
      flag.error = 1;
      response = 0xff;
    }
    return 0;
  case range128(6, 133):
    memory.writeByte(address * 128 + (phase - 7), input);
    checksum ^= input;
    return 0;
  case 134:
    if(checksum != input) {
      flag.error = 1;
      response = 'N';
    }
    return 0;
  case 135: return 0x5c;  //ACK lower
  case 136: return 0x5d;  //ACK upper
  case 137:
    flag.fresh = 0;
    state = State::Idle;
    command = Command::None;
    return response;
  }

  return output;
}

auto MemoryCard::format() -> void {
  constexpr auto frameSize = 0x80;

  const auto fill = [&](u32 address, u32 value) {
    for(u32 offset = 0; offset < frameSize; offset += 4) {
      memory.writeWord(address + offset, value);
    }
  };

  memory.fill(~0);

  for(const auto frame : range(64)) {
    const auto address = frame * frameSize;
    if(frame == 0 || frame == 63) {
      //header frame / test frame
      fill(address, 0);
      memory.writeHalf(address + 0x00, 0x434d);  //"MC"
      memory.writeByte(address + 0x7f, 0x0e);    //checksum
    } else if (frame >= 1 && frame <= 15) {
      //directory frame
      fill(address, 0);
      memory.writeByte(address + 0x00, 0xa0);    //free
      memory.writeHalf(address + 0x08, 0xffff);  //last block
      memory.writeByte(address + 0x7f, 0xa0);    //checksum
    } else if (frame >= 16 && frame <= 35) {
      //broken sector list
      fill(address, 0);
      memory.writeWord(address + 0x00, 0xffffffff); //none
      memory.writeHalf(address + 0x08, 0xffff);     //last block
      memory.writeByte(address + 0x7f, 0x00);       //checksum
    }
  }
}
