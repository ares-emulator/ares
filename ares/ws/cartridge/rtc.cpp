//calculate time between last play of game and current time;
//increment RTC by said amount of seconds
auto Cartridge::RTC::load() -> void {
  n64 timestamp = 0;
  for(auto n : range(8)) timestamp.byte(n) = ram.read(8 + n);
  if(!timestamp) return;  //new save file

  timestamp = time(0) - timestamp;
  while(timestamp--) tickSecond();
}

//save time when game is unloaded
auto Cartridge::RTC::save() -> void {
  n64 timestamp = time(0);
  for(auto n : range(8)) ram.write(8 + n, timestamp.byte(n));
}

auto Cartridge::RTC::tickSecond() -> void {
  if(++second() < 60) return;
  second() = 0;

  if(++minute() < 60) return;
  minute() = 0;

  if(++hour() < 24) return;
  hour() = 0;

  weekday() += 1;
  weekday() %= 7;

  u32 daysInMonth[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  if(year() && (year() % 100) && !(year() % 4)) daysInMonth[1]++;

  if(++day() < daysInMonth[month()]) return;
  day() = 0;

  if(++month() < 12) return;
  month() = 0;

  ++year();
}

auto Cartridge::RTC::checkAlarm() -> void {
  if(!alarm.bit(5)) return;

  if(hour() == alarmHour && minute() == alarmMinute) {
    cpu.raise(CPU::Interrupt::Cartridge);
  } else {
    cpu.lower(CPU::Interrupt::Cartridge);
  }
}

auto Cartridge::RTC::status() -> n8 {
  n8 data;
  data.bit(0,6) = 0;  //unknown
  data.bit(7)   = 1;  //0 = busy; 1 = ready for command
  return data;
}

auto Cartridge::RTC::execute(n8 data) -> void {
  command = data;

  //RESET
  if(command == 0x10) {
    year() = 0;
    month() = 0;
    day() = 0;
    weekday() = 0;
    hour() = 0;
    minute() = 0;
    second() = 0;
  }

  //ALARM_FLAG
  if(command == 0x12) {
    index = 0;
  }

  //SET_DATETIME
  if(command == 0x14) {
    index = 0;
  }

  //GET_DATETIME
  if(command == 0x15) {
    index = 0;
  }

  //SET_ALARM
  if(command == 0x18) {
    index = 0;
  }
}

auto Cartridge::RTC::read() -> n8 {
  n8 data = 0;

  static auto encode = [](n8 data) -> n8 {
    return (data / 10 << 4) + data % 10;
  };

  //GET_DATETIME
  if(command == 0x15) {
    switch(index) {
    case 0: data = encode(year()); break;
    case 1: data = encode(month() + 1); break;
    case 2: data = encode(day() + 1); break;
    case 3: data = encode(weekday()); break;
    case 4: data = encode(hour()); break;
    case 5: data = encode(minute()); break;
    case 6: data = encode(second()); break;
    }
    if(++index >= 7) command = 0;
  }

  return data;
}

auto Cartridge::RTC::write(n8 data) -> void {
  static auto decode = [](n8 data) -> n8 {
    return (data >> 4) * 10 + (data & 0x0f);
  };

  //ALARM_FLAG
  if(command == 0x12) {
    if(data.bit(6)) alarm = data;  //todo: is bit6 really required to be set?
    command = 0;
    checkAlarm();
  }

  //SET_DATETIME
  if(command == 0x14) {
    switch(index) {
    case 0: year()    = decode(data); break;
    case 1: month()   = decode(data) - 1; break;
    case 2: day()     = decode(data) - 1; break;
    case 3: weekday() = decode(data); break;
    case 4: hour()    = decode(data); break;
    case 5: minute()  = decode(data); break;
    case 6: second()  = decode(data); break;
    }
    if(++index >= 7) command = 0;
  }

  //SET_ALARM
  if(command == 0x18) {
    switch(index) {
    case 0: alarmHour   = decode(data.bit(0,6)); break;
    case 1: alarmMinute = decode(data); break;
    }
    if(++index >= 2) command = 0;
  }
}

auto Cartridge::RTC::power() -> void {
  command = 0;
  index = 0;
  alarm = 0;
  alarmHour = 0;
  alarmMinute = 0;
}
