auto BOARD::GS_EEPROM::readByte(n19 address) -> u8 {
  if(mode == Mode::ReadID) {
    const n16 prod_id = id();
    if(address == 0) {
      return prod_id.bit(8,15);
    }
    if(address == 1) {
      return prod_id.bit(0,7);
    }
  } else {
    if(address <= size - 1) {
      return Memory::Writable::read<Byte>(address);
    }
  }
  // TODO: check this
  return 0x00;
}

auto BOARD::GS_EEPROM::writeByte(n19 address, u8 data) -> void {
  if(state == State::WaitStart) {
    if((address == 0x5555) && (data == 0xAA)) {
      state = State::Expect55;
      return;
    }
  }
  if(state == State::Expect55) {
    if((address == 0x2AAA) && (data == 0x55)) {
      state = State::ExpectCommand;
      return;
    }
  }
  if(state == State::ExpectCommand) {
    if(address == 0x5555) {
      switch(data) {
        case 0xA0: {
          state = State::WaitWrite;
          entries = 0;
          sdp = true;
          countdown = delay();
          //FIXME: should be 1 microsecond
          self.setClock(187, true);
        } break;

        case 0x80: {
          state = State::WaitSecondStart;
        } break;

        case 0x90: {
          state = State::WaitStart;
          mode = Mode::ReadID;
        } break;

        case 0xF0: {
          state = State::WaitStart;
          mode = Mode::Normal;
        }
      }
      return;
    }
  }
  if(state == State::WaitWrite) {
    if(entries < 128) {
      write_buffer[entries] = WriteBufferEntry{data, address};
      entries++;
      last_entry_address = address;
    } else {
      debug(unusual, "[Board::GS_EEPROM::writeByte] tried to write more than 128 entries");
    }
    countdown = delay();
    //FIXME: should be 1 microsecond
    self.setClock(187, true);
    return;
  }
  if(state == State::WaitSecondStart) {
    if((address == 0x5555) && (data == 0xAA)) {
      state = State::ExpectSecond55;
      return;
    }
  }
  if(state == State::ExpectSecond55) {
    if((address == 0x2AAA) && (data == 0x55)) {
      state = State::ExpectSecondCommand;
      return;
    }
  }
  if(state == State::ExpectSecondCommand) {
    if(address == 0x5555) {
      switch(data) {
        case 0x20: {
          state = State::WaitStart;
          sdp = false;
        } break;

        case 0x10: {
          state = State::WaitStart;
          Memory::Writable::fill(0xFF);
        } break;

        case 0x60: {
          state = State::WaitStart;
          mode = Mode::ReadID;
        } break;
      }
      return;
    }
  }
  if(!sdp) {
    //I can't find any documentation on how
    //unprotected writes are supposed to work
    //Nothing uses them, so for now this is TODO
  }
}

auto BOARD::GS_EEPROM::clock() -> void {
  if(countdown) {
    countdown--;

    if(countdown == 0) {
      //will need to change if we support
      //EEPROMs with 256-byte pages
      u8 page_buffer[128];
      for(u8 i = 0; i < 128; i++) {
        page_buffer[i] = 0xFF;
      }

      n12 page = 0;
      page = last_entry_address.bit(7,18);

      for(u8 i = 0; i < entries; i++) {
        auto& entry = write_buffer[i];

        if(entry.address.bit(7,18) == page) {
          page_buffer[entry.address.bit(0,6)] = entry.data;
        }
      }

      for(u8 i = 0; i < 128; i++) {
        n19 addr = 0;
        addr.bit(0,6) = i;
        addr.bit(7,18) = page;
        Memory::Writable::write<Byte>(addr, page_buffer[i]);
      }

      state = State::WaitStart;
      entries = 0;
    } else {
      //FIXME: should be 1 microsecond
      self.setClock(187, true);
    }
  }
}

auto BOARD::GS_EEPROM::serialize(serializer& s) -> void {
  Memory::Writable::serialize(s);
  s(typeID);
  s(state);
  s(mode);
  s(sdp);
  s(countdown);
  s(write_buffer);
  s(entries);
  s(last_entry_address);
}
