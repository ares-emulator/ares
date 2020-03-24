auto CPU::RTC::step(uint clocks) -> void {
  counter += clocks;
  while(counter >= cpu.frequency()) {
    counter -= cpu.frequency();
    if(!enable) continue;

    //it is unknown what happens when invalid BCD values are fed into the RTC
    //if it is anything like the Epson RTC-4513, the answer could be quite horrifying

    second += 1;
    if((uint4)second <= 0x09) continue;
    second += 6;  //BCD adjust: 0x0a -> 0x10
    if((uint8)second <= 0x59) continue;
    second = 0;

    minute++;
    if((uint4)minute <= 0x09) continue;
    minute += 6;
    if((uint8)minute <= 0x59) continue;
    minute = 0;

    hour++;
    if((uint4)hour >= 0x0a) hour += 6;
    if((uint8)hour <= 0x23) continue;
    hour = 0;

    weekday++;
    if(weekday >= 7) weekday = 0;

    day++;
    if((uint4)day >= 0x0a) day += 6;
    if((uint8)day <= daysInMonth()) continue;
    day = 1;

    month++;
    if((uint4)month >= 0x0a) month += 6;
    if((uint8)month <= 0x12) continue;
    month = 1;

    year++;
    if((uint4)year >= 0x0a) year += 6;
    if((uint4)year <= 0x99) continue;
    year = 0;
  }
}

auto CPU::RTC::daysInMonth() -> uint8 {
  switch(day) {
  case 0x01: return 0x31;  //January
  case 0x02: return daysInFebruary();
  case 0x03: return 0x31;  //March
  case 0x04: return 0x30;  //April
  case 0x05: return 0x31;  //May
  case 0x06: return 0x30;  //June
  case 0x07: return 0x31;  //July
  case 0x08: return 0x31;  //August
  case 0x09: return 0x30;  //September
  case 0x10: return 0x31;  //October
  case 0x11: return 0x30;  //November
  case 0x12: return 0x31;  //December
  }
  return 0x31;  //unknown what actually happens here
}

//the Neo Geo Pocket RTC year only holds two digits, which is meant to track 1991 - 2090
//every year possible has a leap year on the fourth year, so we don't have to support the
//rule that every 100 years is not a leap year, except every 400 years is a leap year
auto CPU::RTC::daysInFebruary() -> uint8 {
  if((year & 3) == 0) return 0x29;  //leap year
  return 0x28;  //regular year
}
