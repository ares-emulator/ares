//calculate time between last play of game and current time;
//increment RTC by said amount of seconds
auto Cartridge::RTC::load() -> void {
  n64 timestamp = 0;
  for(auto n : range(8)) timestamp.byte(n) = ram.read(8 + n);
  if(!timestamp || !(timestamp + 1)) return;  //new save file

  if(status() & 0x15) {
    // These status bits are always 0; reset state on invalid status.
    initRegs(false);
    return;
  }

  timestamp = time(0) - timestamp;
  // prevent insurmountable slowdown by limiting skips to 5 years
  if(timestamp < 60*60*24*365*5) {
    while(timestamp--) tickSecond();
  }
}

//save time when game is unloaded
auto Cartridge::RTC::save() -> void {
  n64 timestamp = time(0);
  for(auto n : range(8)) ram.write(8 + n, timestamp.byte(n));
}

auto Cartridge::RTC::tickSecond() -> void {
  static auto bcdIncrement = [](n8& data) -> n8 {
    if ((data & 0x0F) >= 0x09) {
      data = (data & 0xF0) + 0x10;
    } else {
      data++;
    }
    return data;
  };

  if(bcdIncrement(second()) < 0x60) return;
  second() = 0;

  if(bcdIncrement(minute()) < 0x60) return;
  minute() = 0;

  if(status() & 0x40) {
    // 24-hour clock
    if((bcdIncrement(hour()) & 0x7F) < 0x24) return;
    hour().bit(0,5) = 0;
  } else {
    // 12-hour clock
    if((bcdIncrement(hour()) & 0x7F) < 0x12) return;
    hour().bit(0,5) = 0;
    hour().bit(7) ^= 1;
  }

  weekday() += 1;
  weekday() %= 7;

  u32 bcdDaysInMonth[12] = {0x31, 0x28, 0x31, 0x30, 0x31, 0x30, 0x31, 0x31, 0x30, 0x31, 0x30, 0x31};
  if(year() && !(year() & 3)) bcdDaysInMonth[1]++;

  if(bcdIncrement(day()) <= bcdDaysInMonth[month()]) return;
  day() = 1;

  if(bcdIncrement(month()) <= 0x12) return;
  month() = 1;

  bcdIncrement(year());
}

auto Cartridge::RTC::checkAlarm() -> void {
  // TODO: A lot of this is vaguely informed guesswork.
  if(status() & 0x08) {
    // Per-minute edge/steady
    if(counter < 256) cpu.irqLevel(CPU::Interrupt::Cartridge, 1);
    if(status() & 0x02 && second() == 0x30 && counter == 0) cpu.irqLevel(CPU::Interrupt::Cartridge, 0);
  } else if(status() & 0x02) {
    // Selected frequency steady
    n16 duty = (counter << 1) ^ 0xFFFF;
    n16 mask = (alarmHour() | (alarmMinute() << 8));
    cpu.irqLevel(CPU::Interrupt::Cartridge, (duty & mask) != 0);
  } else if(status() & 0x20) {
    // Alarm
    if(status() & 0x40) {
      // 24-hour clock
      cpu.irqLevel(CPU::Interrupt::Cartridge, (hour() & 0x3F) == (alarmHour() & 0x3F) && (minute() & 0x7F) == (alarmMinute() & 0x7F));
    } else {
      // 12-hour clock
      cpu.irqLevel(CPU::Interrupt::Cartridge, (hour() & 0xBF) == (alarmHour() & 0xBF) && (minute() & 0x7F) == (alarmMinute() & 0x7F));
    }
  } else {
    cpu.irqLevel(CPU::Interrupt::Cartridge, 0);
  }
}

auto Cartridge::RTC::controlRead() -> n8 {
  n8 data = 0;
  data.bit(0,3) = command;
  data.bit(4)   = active;
  data.bit(7)   = ready;
  return data;
}

auto Cartridge::RTC::controlWrite(n5 data) -> void {
  command = data.bit(0,3);
  active = data.bit(4);

  if(active) switch(command & 0x0E) {
    case 0x00: { // RESET
      initRegs(true);
      active = 0;
    } break;
    case 0x02: { // ALARM_FLAG
      index = 0;
    } break;
    case 0x04: { // DATETIME
      index = 0;
    } break;
    case 0x06: { // TIME
      index = 0;
    } break;
    case 0x08: { // ALARM
      index = 0;
    } break;
    case 0x0A: { // no-op
      index = 0;
    } break;
  }

  ready = 1;
  if(active &&  command.bit(0)) fetch();
  if(active && !command.bit(0)) write(fetchedData);
}

auto Cartridge::RTC::fetch() -> void {
  n8 data = 0;

  switch(command & 0x0E) {
    case 0x02: { // STATUS
      data = status();
      status().bit(7) = 0;
      index = 1;
      active = 0;
    } break;
    case 0x04: { // DATETIME
      switch(index) {
      case 0: data = year(); break;
      case 1: data = month(); break;
      case 2: data = day(); break;
      case 3: data = weekday(); break;
      case 4: data = hour(); break;
      case 5: data = minute(); break;
      case 6: data = second(); break;
      }
      if(++index >= 7) active = 0;
    } break;
    case 0x06: { // TIME
      switch(index) {
      case 0: data = hour(); break;
      case 1: data = minute(); break;
      case 2: data = second(); break;
      }
      if(++index >= 3) active = 0;
    } break;
    case 0x08: { // ALARM
      switch(index) {
      case 0: data = alarmHour(); break;
      case 1: data = alarmMinute(); break;
      }
      if(++index >= 2) active = 0;
    } break;
    case 0x0A: { // no-op
      data = 0xFF;
      if(++index >= 2) active = 0;
    } break;
  }

  ready = 1;
  fetchedData = data;
}

auto Cartridge::RTC::read() -> n8 {
  n8 data = fetchedData;

  if(!active) ready = 0;
  if(active && command.bit(0)) fetch();

  return data;
}

auto Cartridge::RTC::write(n8 data) -> void {
  fetchedData = data;

  if(!active) ready = 0;
  if(active && !command.bit(0)) switch(command & 0x0E) {
    case 0x02: { // STATUS
      status().bit(6) = data.bit(6);
      status().bit(5) = data.bit(5);
      status().bit(3) = data.bit(3);
      status().bit(1) = data.bit(1);
      active = 0;
    } break;
    case 0x04: { // DATETIME
      switch(index) {
      case 0: year()            = data; break;
      case 1: month()           = data; break;
      case 2: day()             = data; break;
      case 3: weekday()         = data; break;
      case 4: hour()            = data; break;
      case 5: minute()          = data; break;
      case 6: second().bit(0,6) = data.bit(0,6); break;
      }
      if(++index >= 7) active = 0;
    } break;
    case 0x06: { // TIME
      switch(index) {
      case 0: hour()            = data; break;
      case 1: minute()          = data; break;
      case 2: second().bit(0,6) = data.bit(0,6); break;
      }
      if(++index >= 3) active = 0;
    } break;
    case 0x08: { // ALARM
      switch(index) {
      case 0: alarmHour()   = data; break;
      case 1: alarmMinute() = data; break;
      }
      if(++index >= 2) active = 0;
    } break;
    case 0x0A: { // no-op
      if(++index >= 2) active = 0;
    } break;
    default: {
      active = 0;
    } break;
  }
}

auto Cartridge::RTC::initRegs(bool reset) -> void {
  year() = 0;
  month() = 1;
  day() = 1;
  weekday() = 0;
  hour() = 0;
  minute() = 0;
  second() = 0;
  status() = reset ? 0x00 : 0x82;
  alarmHour() = 0x00;
  alarmMinute() = reset ? 0x00 : 0x80;
}

auto Cartridge::RTC::power() -> void {
  Thread::create(32'768, {&Cartridge::RTC::main, this});
  
  command = 0;
  active = 0;
  ready = 0;
  index = 0;
  counter = 0;
  fetchedData = 0xFF;
}

auto Cartridge::RTC::reset() -> void {
  Thread::destroy();

  ram.reset();
}

auto Cartridge::RTC::main() -> void {
  if(++counter == 0) tickSecond();
  checkAlarm();
  step(1);
}

auto Cartridge::RTC::step(u32 clocks) -> void {
  Thread::step(clocks);
  synchronize(cpu);
}

