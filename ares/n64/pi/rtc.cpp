auto PI::BBRTC::load() -> void {
  ram.allocate(0x10);
  if(auto fp = system.pak->read("time.rtc")) {
    ram.load(fp);
  }

  //byte 0 to 7 = raw rtc data
  n64 check = 0;
  for(auto n : range(8)) check.byte(n) = ram.read<Byte>(n);
  if(!~check) return;  //new save file

  //check for invalid time info, if invalid, set time info to something invalid and ignore the rest
  if (!valid()) {
    for(auto n : range(8)) ram.write<Byte>(n, 0xff);
    return;
  }

  //byte 8 to 15 = timestamp of when the last save was made
  n64 timestamp = 0;
  for(auto n : range(8)) timestamp.byte(n) = ram.read<Byte>(8 + n);
  if(!~timestamp) return;  //new save file

  //update based on the amount of time that has passed since the last save
  timestamp = time(0) - timestamp;
  while(timestamp--) tickSecond();
}

auto PI::BBRTC::reset() -> void {
  ram.reset();
}

auto PI::BBRTC::save() -> void {
  n64 timestamp = time(0);
  for(auto n : range(8)) ram.write<Byte>(8 + n, timestamp.byte(n));

  if(auto fp = system.pak->write("time.rtc")) {
    ram.save(fp);
  }
}

auto PI::BBRTC::serialize(serializer& s) -> void {
  s(ram);

  s(prev_linestate);

  s(addr);
  s(data_addr);
  s(bit_count);
  s(byte_count);
  s(read);
  s(enabled);

  s(state);
}

auto PI::BBRTC::tick() -> void {
  auto new_clock = pi.bb_gpio.rtc_clock.lineOut;
  auto new_data = pi.bb_gpio.rtc_data.lineOut;

  n2 linestate = 0;
  linestate.bit(0) = new_data;
  linestate.bit(1) = new_clock;

  auto clock_high = [](n2 line) -> b1 { return line.bit(1); };
  auto clock_pos_edge = [&](n2 prev, n2 line) { return !clock_high(prev) & clock_high(line); };

  auto data_high = [](n2 line) -> b1 { return line.bit(0); };
  auto start_bit = [&](n2 prev, n2 line) { return (clock_high(prev) & data_high(prev)) & (clock_high(line) & !data_high(line)); };
  auto stop_bit = [&](n2 prev, n2 line) { return (clock_high(prev) & !data_high(prev)) & (clock_high(line) & data_high(line)); };

  if(start_bit(prev_linestate, linestate)) {
    printf("Start bit\n");
    enabled = 1;
    state = State::Address;
    return;
  }

  if(stop_bit(prev_linestate, linestate)) {
    printf("Stop bit\n");
    bit_count = 0;
    byte_count = 0;
    enabled = 0;
  }

  if(enabled & clock_pos_edge(prev_linestate, linestate)) {
    switch(state) {
      case State::Address: {
        if(bit_count < 7) {
          addr.bit(7 - bit_count - 1) = data_high(linestate);
          bit_count += 1;
        } else if(bit_count == 7) {
          printf("read: %d %d (%d)\n", (u32)linestate, (u32)data_high(linestate), (u32)!data_high(linestate));
          read = !data_high(linestate);
          bit_count += 1;
        } else {
          //ack
          printf("Transmitting ack\n");
          pi.bb_gpio.rtc_data.lineIn = 0;
          printf("%s device addr: 0x%02x\n", read ? "read from" : "write to", (u32)addr);
          bit_count = 0;
          state = read ? State::Read : State::Write;
        }
      } break;
      case State::Read: {
        if(bit_count < 8) {
          printf("read from address %d bit %d\n", (u32)data_addr.bit(0,2), (u32)bit_count);
          auto byte = ram.read<Byte>(data_addr.bit(0,2));
          printf("byte = %02X\n", (u32)byte);
          auto bit = ((n8)byte).bit(8 - bit_count - 1);
          printf("transmitting bit: %d\n", (u8)bit);
          pi.bb_gpio.rtc_data.lineIn = bit;
          bit_count += 1;
        } else {
          //ack
          printf("transmitting ack bit\n");
          pi.bb_gpio.rtc_data.lineIn = 0;
          data_addr += 1;
          bit_count = 0;
        }
      } break;
      case State::Write: {
        if(byte_count == 0) {
          if(bit_count < 8) {
            data_addr.bit(8 - bit_count - 1) = data_high(linestate);
            bit_count += 1;
          } else {
            //ack
            printf("transmitting ack bit\n");
            pi.bb_gpio.rtc_data.lineIn = 0;
            bit_count = 0;
            byte_count += 1;
          }
        } else {
          if(bit_count < 8) {
            n8 byte = ram.read<Byte>(data_addr.bit(0,2));
            byte.bit(8 - bit_count - 1) = data_high(linestate);
            ram.write<Byte>(data_addr.bit(0,2), byte);
            bit_count += 1;
          } else {
            printf("transmitting ack bit\n");
            data_addr += 1;
            bit_count = 0;
            byte_count += 1;
          }
        }
      } break;
    }
  }

  prev_linestate = linestate;
}

auto PI::BBRTC::tickClock() -> void {
  tickSecond();
  queue.remove(Queue::BB_RTC_Tick);
  queue.insert(Queue::BB_RTC_Tick, system.frequency());
}

auto PI::BBRTC::tickSecond() -> void {
  printf("tick second\n");
  /*if (!valid()) return;

  //second
  tick(5);
  if(ram.read<Byte>(5) < 0x60) return;
  ram.write<Byte>(5, 0);

  //minute
  tick(4);
  if(ram.read<Byte>(4) < 0x60) return;
  ram.write<Byte>(4, 0);

  //hour
  tick(3);
  if(ram.read<Byte>(3) < 0x24) return;
  ram.write<Byte>(3, 0);

  //day
  tick(2);
  if(ram.read<Byte>(2) <= BCD::encode(chrono::daysInMonth(BCD::decode(ram.read<Byte>(1)), BCD::decode(ram.read<Byte>(0))))) return;
  ram.write<Byte>(2, 1);

  //month
  tick(1);
  if(ram.read<Byte>(1) <= 0x12) return;
  ram.write<Byte>(1, 1);

  //year
  tick(0);*/
}

auto PI::BBRTC::valid() -> bool {
  //check validity of ram rtc data (if it's BCD valid or not)
  for(auto n : range(6)) {
    if((ram.read<Byte>(n) & 0x0f) >= 0x0a) return false;
  }

  //check for valid values of each byte
  //year
  if(ram.read<Byte>(0) >= 0xa0) return false;
  //second
  if(ram.read<Byte>(5) >= 0x60) return false;
  //minute
  if(ram.read<Byte>(4) >= 0x60) return false;
  //hour
  if(ram.read<Byte>(3) >= 0x24) return false;
  //month
  if(ram.read<Byte>(1) > 0x12) return false;
  if(ram.read<Byte>(1) < 1) return false;
  //day
  if(ram.read<Byte>(2) < 1) return false;
  if(ram.read<Byte>(2) > BCD::encode(chrono::daysInMonth(BCD::decode(ram.read<Byte>(1)), BCD::decode(ram.read<Byte>(0))))) return false;

  //everything is valid
  return true;
}

