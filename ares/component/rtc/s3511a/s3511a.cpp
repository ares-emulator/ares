#include <ares/ares.hpp>
#include "s3511a.hpp"

namespace ares {

#include "serialization.cpp"

//calculate time between last play of game and current time;
//increment RTC by said amount of seconds
auto S3511A::load() -> void {
  n64 timestamp = 0;
  for(auto n : range(8)) timestamp.byte(n) = data[8 + n];
  if(!timestamp || !(timestamp + 1)) return initRegs(false);  //new save file

  if(status() & 0x15) {
    //these status bits are always 0; reset state on invalid status.
    initRegs(false);
    return;
  }

  timestamp = time(0) - timestamp;
  //prevent insurmountable slowdown by limiting skips to 5 years
  if(timestamp < 60*60*24*365*5) {
    while(timestamp--) tickSecond();
  }
}

//save time when game is unloaded
auto S3511A::save() -> void {
  n64 timestamp = time(0);
  for(auto n : range(8)) data[8 + n] = timestamp.byte(n);
}

auto S3511A::power() -> void {
  cs = 0;
  sio = 0;
  sck = 0;
  inBuffer = 0;
  outBuffer = 0;
  shift = 0;
  index = 0;
  rwSelect = 0;
  regSelect = 0;
  cmdLatched = 0;
  counter = 0;
}

auto S3511A::tickSecond() -> void {
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
    //24-hour clock
    if((bcdIncrement(hour()) & 0x7F) < 0x24) return;
    hour().bit(0,5) = 0;
  } else {
    //12-hour clock
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

auto S3511A::checkAlarm() -> void {
  //todo: a lot of this is vaguely informed guesswork.
  if(status() & 0x08) {
    //per-minute edge/steady
    if(counter < 256) irqLevel(1);
    if(status() & 0x02 && second() == 0x30 && counter == 0) irqLevel(0);
  } else if(status() & 0x02) {
    //selected frequency steady
    n16 duty = (counter << 1) ^ 0xFFFF;
    n16 mask = (alarmHour() | (alarmMinute() << 8));
    irqLevel((duty & mask) != 0);
  } else if(status() & 0x20) {
    //alarm
    if(status() & 0x40) {
      //24-hour clock
      irqLevel((hour() & 0x3F) == (alarmHour() & 0x3F) && (minute() & 0x7F) == (alarmMinute() & 0x7F));
    } else {
      //12-hour clock
      irqLevel((hour() & 0xBF) == (alarmHour() & 0xBF) && (minute() & 0x7F) == (alarmMinute() & 0x7F));
    }
  } else {
    irqLevel(0);
  }
}

auto S3511A::readSIO() -> n1 {
  return sio | ~rwSelect | ~cs;
}

auto S3511A::writeCS(n1 data) -> void {
  n1 csPrev = cs;
  cs = data;

  //clear command status on falling edge
  if(csPrev == 1 && cs == 0) {
    inBuffer = 0x00;
    shift = 0;
    index = 0;
    rwSelect = 0;
    cmdLatched = 0;
  }
}

auto S3511A::writeSIO(n1 data) -> void {
  sio = data;
}

auto S3511A::writeSCK(n1 data) -> void {
  n1 sckPrev = sck | ~cs;
  sck = data;

  //only read SIO data if CS is raised
  if(!cs) return;

  //process command and output data on falling edge of SCK
  if(sckPrev == 1 && sck == 0) {
    if(!cmdLatched && shift == 0) writeCommand();
    if(cmdLatched && rwSelect == 1 && shift == 0) readRegister();
    sio = outBuffer.bit(shift);
  }

  //sample RTC command data on rising edge of SCK
  if(sckPrev == 0 && sck == 1) {
    //add bit into input buffer
    inBuffer.bit(shift) = sio;
    shift++;
    if(cmdLatched && rwSelect == 0 && shift == 0) writeRegister();
  }
}

auto S3511A::writeCommand() -> void {
  n4 magic = inBuffer >> 4;
  n4 cmd = inBuffer;
  if(magic != 0b0110) {
    //flip bit order of command
    magic = cmd;
    cmd.bit(0) = inBuffer.bit(7);
    cmd.bit(1) = inBuffer.bit(6);
    cmd.bit(2) = inBuffer.bit(5);
    cmd.bit(3) = inBuffer.bit(4);
  }
  if(magic != 0b0110) return;  //invalid command
  rwSelect = cmd;
  regSelect = cmd >> 1;
  cmdLatched = 1;

  //process any registers that have an effect on write without a payload
  if(rwSelect == 1) return;
  switch(regSelect) {
  case 0:
    //RESET
    initRegs(true);
    break;
  default:
    break;
  }
}

auto S3511A::readRegister() -> void {
  switch(regSelect) {
  case 1:
    //STATUS
    outBuffer = status();
    break;
  case 2: {
    //DATETIME
    switch(index) {
    case 0:  outBuffer = year();    index++; break;
    case 1:  outBuffer = month();   index++; break;
    case 2:  outBuffer = day();     index++; break;
    case 3:  outBuffer = weekday(); index++; break;
    case 4:  outBuffer = hour();    index++; break;
    case 5:  outBuffer = minute();  index++; break;
    default: outBuffer = second();           break;
    }
    break;
  }
  case 3: {
    //TIME
    switch(index) {
    case 0:  outBuffer = hour();   index++; break;
    case 1:  outBuffer = minute(); index++; break;
    default: outBuffer = second();          break;
    }
    break;
  }
  case 4: {
    //ALARM
    switch(index) {
    case 0:  outBuffer = alarmHour();   index++; break;
    default: outBuffer = alarmMinute();          break;
    }
  }
  default:
    outBuffer = 0xff;
    break;
  }
}

auto S3511A::writeRegister() -> void {
  switch(regSelect) {
  case 1:
    //STATUS
    status().bit(1)  = inBuffer.bit(1);
    status().bit(3)  = inBuffer.bit(3);
    status().bit(5)  = inBuffer.bit(5);
    status().bit(6)  = inBuffer.bit(6);
    status().bit(7) &= inBuffer.bit(7);
    break;
  case 2: {
    //DATETIME
    switch(index) {
    case 0:  year()             = inBuffer; index++; break;
    case 1:  month()            = inBuffer; index++; break;
    case 2:  day()              = inBuffer; index++; break;
    case 3:  weekday()          = inBuffer; index++; break;
    case 4:  hour()             = inBuffer; index++; break;
    case 5:  minute()           = inBuffer; index++; break;
    default: second().bit(0,6)  = inBuffer.bit(0,6); break;
    }
    break;
  }
  case 3: {
    //TIME
    switch(index) {
    case 0:  hour()             = inBuffer; index++; break;
    case 1:  minute()           = inBuffer; index++; break;
    default: second().bit(0,6)  = inBuffer.bit(0,6); break;
    }
    break;
  }
  case 4: {
    //ALARM
    switch(index) {
    case 0:  alarmHour()   = inBuffer; index++; break;
    default: alarmMinute() = inBuffer;          break;
    }
  }
  default:
    break;
  }
}

auto S3511A::initRegs(bool reset) -> void {
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

}
