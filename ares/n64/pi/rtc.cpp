auto PI::BBRTC::load() -> void {
  ram.allocate(0x10);
  if(auto fp = system.pak->read("time.rtc")) {
    ram.load(fp);
  }

  //check for invalid time info, if invalid, set time info to something invalid and ignore the rest
  if (!valid()) {
    for(auto n : range(8)) ram.write<Byte>(n, 0x00);
    set_of(1);
    set_out(1);
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

  s(stored_linestate);

  s(addr);
  s(data_addr);
  s(bit_count);
  s(byte_count);
  s(read);
  s(enabled);

  s(phase);
  s(state);
}

auto PI::BBRTC::tick() -> void {
  //I²C lines have pullups
  if(!pi.bb_gpio.rtc_clock.outputEnable)
    pi.bb_gpio.rtc_clock.lineOut = 1;
  if(!pi.bb_gpio.rtc_data.outputEnable)
    pi.bb_gpio.rtc_data.lineOut = 1;

  auto new_clock = pi.bb_gpio.rtc_clock.lineOut;
  auto new_data = pi.bb_gpio.rtc_data.lineOut;

  n2 linestate = 0;
  linestate.bit(0) = new_data;
  linestate.bit(1) = new_clock;

  n2 prev_linestate = stored_linestate;
  stored_linestate = linestate;

  auto clock_high = [](n2 line) -> b1 { return line.bit(1); };
  auto clock_pos_edge = [&](n2 prev, n2 line) { return !clock_high(prev) & clock_high(line); };
  auto clock_neg_edge = [&](n2 prev, n2 line) { return clock_high(prev) & !clock_high(line); };
  auto clock_edge = [&](n2 prev, n2 line) { return clock_pos_edge(prev, line) | clock_neg_edge(prev, line); };

  auto data_high = [](n2 line) -> b1 { return line.bit(0); };
  //this isn't how start bits work in most versions of I²C, but libultra expects this behaviour
  auto start_bit = [&](n2 prev, n2 line) { return (data_high(prev)) & (clock_high(line) & !data_high(line)); };
  auto stop_bit = [&](n2 prev, n2 line) { return (clock_high(prev) & !data_high(prev)) & (clock_high(line) & data_high(line)); };

  if(start_bit(prev_linestate, linestate)) {
    bit_count = 0;
    byte_count = 0;
    state = State::Address;
    enabled = 1;
    return;
  }

  if(stop_bit(prev_linestate, linestate)) {
    bit_count = 0;
    byte_count = 0;
    state = State::Address;
    enabled = 0;
  }

  if(clock_pos_edge(prev_linestate, linestate)) {
    phase = Phase::Sample;
  } else if(clock_neg_edge(prev_linestate, linestate)) {
    phase = Phase::Setup;
  }

  if(enabled & clock_edge(prev_linestate, linestate)) {
    switch(state) {
      case State::Address: {
        switch(phase) {
          case Phase::Setup: {
            //transmit ack
            pi.bb_gpio.rtc_data.lineIn = 0;
          } break;
          case Phase::Sample: {
            if(bit_count < 7) {
              addr.bit(7 - bit_count - 1) = data_high(linestate);
              bit_count += 1;
            } else if(bit_count == 7) {
              read = data_high(linestate);
              bit_count += 1;
            } else {
              //ack
              bit_count = 0;
              state = read ? State::Read : State::Write;
            }
          } break;
        }
      } break;
      case State::Read: {
        switch(phase) {
          case Phase::Setup: {
            if(bit_count < 8) {
              pi.bb_gpio.rtc_data.lineIn = ((n8)ram.read<Byte>(data_addr.bit(0,2))).bit(8 - bit_count - 1);
              bit_count += 1;
            } else {
              //ack
              data_addr += 1;
              bit_count = 0;
            }
          } break;
          case Phase::Sample: {
            if(bit_count == 8) {
              //receive ack
            }
          } break;
        }
      } break;
      case State::Write: {
        switch(phase) {
          case Phase::Setup: {
            //transmit ack
            pi.bb_gpio.rtc_data.lineIn = 0;
          } break;
          case Phase::Sample: {
            if(byte_count == 0) {
              if(bit_count < 8) {
                data_addr.bit(8 - bit_count - 1) = data_high(linestate);
                bit_count += 1;
              } else {
                //ack
                bit_count = 0;
                byte_count += 1;
              }
            } else {
              if(bit_count < 8) {
                n8 byte = ram.read<Byte>(data_addr.bit(0,2));

                //byte 7 bit 6 is hardcoded to 0
                if((data_addr.bit(0,2) != 7) || (bit_count != 1))
                  byte.bit(8 - bit_count - 1) = data_high(linestate);
                else
                  byte.bit(8 - bit_count - 1) = 0;

                ram.write<Byte>(data_addr.bit(0,2), byte);
                bit_count += 1;
              } else {
                data_addr += 1;
                bit_count = 0;
                byte_count += 1;
              }
            }
          } break;
        }
      } break;
    }
  }
}

auto PI::BBRTC::tickClock() -> void {
  tickSecond();
  queue.remove(Queue::BB_RTC_Tick);
  queue.insert(Queue::BB_RTC_Tick, system.frequency());
}

auto PI::BBRTC::tickSecond() -> void {
  if(!valid()) return;
  if(st()) return;

  //seconds
  set_seconds(BCD::encode(BCD::decode(seconds()) + 1));
  if(seconds() <= 0x59) return;
  set_seconds(0);

  //minutes
  set_minutes(BCD::encode(BCD::decode(minutes()) + 1));
  if(minutes() <= 0x59) return;
  set_minutes(0);

  //hours
  set_hours(BCD::encode(BCD::decode(hours()) + 1));
  if(hours() <= 0x23) return;
  set_hours(0);

  //date
  set_day(day() + 1);
  set_date(BCD::encode(BCD::decode(date()) + 1));
  if(day() == 0) set_day(1);
  if(date() < BCD::encode(chrono::daysInMonth(month(), years()))) return;
  set_date(1);

  //month
  set_month(BCD::encode(BCD::decode(month()) + 1));
  if(month() <= 0x11) return;
  set_month(1);

  //years
  set_years(BCD::encode(BCD::decode(years()) + 1));
  if(years() <= 0x99) return;
  set_years(0);
  
  //century
  if(!ceb()) return;
  set_cb(!cb());
}

inline auto PI::BBRTC::seconds() -> n7 {
  return ((n8)ram.read<Byte>(0)).bit(0,6);
}

inline auto PI::BBRTC::minutes() -> n7 {
  return ((n8)ram.read<Byte>(1)).bit(0,6);
}

inline auto PI::BBRTC::hours() -> n6 {
  return ((n8)ram.read<Byte>(2)).bit(0,5);
}

inline auto PI::BBRTC::day() -> n3 {
  return ((n8)ram.read<Byte>(3)).bit(0,2);
}

inline auto PI::BBRTC::date() -> n6 {
  return ((n8)ram.read<Byte>(4)).bit(0,5);
}

inline auto PI::BBRTC::month() -> n5 {
  return ((n8)ram.read<Byte>(5)).bit(0,4);
}

inline auto PI::BBRTC::years() -> n8 {
  return ((n8)ram.read<Byte>(6)).bit(0,7);
}

inline auto PI::BBRTC::st() -> n1 {
  return ((n8)ram.read<Byte>(0)).bit(7);
}

inline auto PI::BBRTC::of() -> n1 {
  return ((n8)ram.read<Byte>(1)).bit(7);
}

inline auto PI::BBRTC::ceb() -> n1 {
  return ((n8)ram.read<Byte>(2)).bit(7);
}

inline auto PI::BBRTC::cb() -> n1 {
  return ((n8)ram.read<Byte>(2)).bit(6);
}

inline auto PI::BBRTC::out() -> n1 {
  return ((n8)ram.read<Byte>(7)).bit(7);
}

inline auto PI::BBRTC::set_seconds(n7 value) -> void {
  n8 byte = ram.read<Byte>(0);
  byte.bit(0,6) = value;
  return ram.write<Byte>(0, byte);
}

inline auto PI::BBRTC::set_minutes(n7 value) -> void {
  n8 byte = ram.read<Byte>(0);
  byte.bit(0,6) = value;
  return ram.write<Byte>(1, byte);
}

inline auto PI::BBRTC::set_hours(n6 value) -> void {
  n8 byte = ram.read<Byte>(0);
  byte.bit(0,5) = value;
  return ram.write<Byte>(2, byte);
}

inline auto PI::BBRTC::set_day(n3 value) -> void {
  n8 byte = ram.read<Byte>(0);
  byte.bit(0,2) = value;
  return ram.write<Byte>(3, byte);
}

inline auto PI::BBRTC::set_date(n6 value) -> void {
  n8 byte = ram.read<Byte>(0);
  byte.bit(0,5) = value;
  return ram.write<Byte>(4, byte);
}

inline auto PI::BBRTC::set_month(n5 value) -> void {
  n8 byte = ram.read<Byte>(0);
  byte.bit(0,4) = value;
  return ram.write<Byte>(5, byte);
}

inline auto PI::BBRTC::set_years(n8 value) -> void {
  n8 byte = ram.read<Byte>(0);
  byte.bit(0,7) = value;
  return ram.write<Byte>(6, byte);
}

inline auto PI::BBRTC::set_st(n1 value) -> void {
  n8 byte = ram.read<Byte>(0);
  byte.bit(7) = value;
  return ram.write<Byte>(0, byte);
}

inline auto PI::BBRTC::set_of(n1 value) -> void {
  n8 byte = ram.read<Byte>(1);
  byte.bit(7) = value;
  return ram.write<Byte>(1, byte);
}

inline auto PI::BBRTC::set_ceb(n1 value) -> void {
  n8 byte = ram.read<Byte>(2);
  byte.bit(7) = value;
  return ram.write<Byte>(2, byte);
}

inline auto PI::BBRTC::set_cb(n1 value) -> void {
  n8 byte = ram.read<Byte>(2);
  byte.bit(6) = value;
  return ram.write<Byte>(2, byte);
}

inline auto PI::BBRTC::set_out(n1 value) -> void {
  n8 byte = ram.read<Byte>(7);
  byte.bit(7) = value;
  return ram.write<Byte>(7, byte);
}

auto PI::BBRTC::valid() -> bool {
  //check validity of ram rtc data (if it's BCD valid or not)
  const struct {
    n4 mask;
    n4 min;
    n4 max;
  } nibbles[8][2] = {
    { { 0b0111, 0, 5 }, { 0b1111, 0, 9 } },
    { { 0b0111, 0, 5 }, { 0b1111, 0, 9 } },
    { { 0b0011, 0, 2 }, { 0b1111, 0, 9 } },
    { { 0b0000, 0, 0 }, { 0b0111, 1, 7 } },
    { { 0b0011, 0, 3 }, { 0b1111, 0, 9 } },
    { { 0b0001, 0, 1 }, { 0b1111, 0, 9 } },
    { { 0b1111, 0, 9 }, { 0b1111, 0, 9 } },
    { { 0b0100, 0, 0 }, { 0b0000, 0, 0 } },
  };

  for(auto n : range(8)) {
    n8 byte = ram.read<Byte>(n);

    for(auto h : range(2)) {
      n4 nibble = byte >> ((1 - h) * 4);

      auto& entry = nibbles[n][h];

      auto masked = nibble & entry.mask;

      if((masked < entry.min) || (masked > entry.max)) return false;
    }
  }

  //check for valid values of each byte
  //seconds
  if(seconds() > 0x59) return false;
  //minutes
  if(minutes() > 0x59) return false;
  //hours
  if(hours() > 0x23) return false;
  //byte 3 is checked already above
  //date
  if(date() > 0x31) return false;
  if(date() < 0x01) return false;
  //month
  if(month() > 0x12) return false;
  if(month() < 0x01) return false;
  //years
  if(years() > 0x99) return false;

  auto day = date();
  auto month = this->month();
  auto year = years();

  if(day > BCD::encode(chrono::daysInMonth(BCD::decode(month), BCD::decode(year)))) return false;

  //everything is valid
  return true;
}

