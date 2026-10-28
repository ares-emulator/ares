#include <ares/ares.hpp>
#include "upd4990a.hpp"

namespace ares {

#include "serialization.cpp"

auto uPD4990A::load() -> void {
  n64 saved = timestamp();
  if(!saved || !(saved + 1)) {
    time_t t = time(0);
    struct tm tmm = *localtime(&t);
    second() = BCD::encode(tmm.tm_sec);
    minute() = BCD::encode(tmm.tm_min);
    hour()   = BCD::encode(tmm.tm_hour);
    day()    = BCD::encode(tmm.tm_mday);
    setWeekday(tmm.tm_wday);
    setMonth(tmm.tm_mon + 1);
    year()   = BCD::encode(tmm.tm_year % 100);
    return;
  }

  //catch up elapsed seconds since last save
  time_t now = time(0);
  time_t prev = (time_t)saved;
  if(now > prev) {
    n64 elapsed = now - prev;
    while(elapsed--) tickSecond();
  }
}

auto uPD4990A::save() -> void {
  n64 t = time(0);
  for(auto n : range(8)) data[8 + n] = t.byte(n);
}

auto uPD4990A::power() -> void {
  clockPin = 0;
  strobePin = 0;
  for(auto& b : shiftRegister) b = 0;
  commandRegister = 0;
  command = RegisterHold;
  outputBit = 0;
  timePulseOutput = 0;
  counter = 0;
  timePulseMode = TimePulse64Hz;
  timePulseCounter = 0;
}

auto uPD4990A::tick() -> void {
  if(++counter == 0) tickSecond();

  u32 halfPeriod = 256;
  if(timePulseMode == TimePulse256Hz)  halfPeriod = 64;
  if(timePulseMode == TimePulse2048Hz) halfPeriod = 8;
  if(timePulseMode == 0x7)             halfPeriod = 4;
  if(timePulseMode == TimePulse1s)  halfPeriod = 16'384;
  if(timePulseMode == TimePulse10s) halfPeriod = 163'840;
  if(timePulseMode == TimePulse30s) halfPeriod = 491'520;
  if(timePulseMode == TimePulse60s) halfPeriod = 983'040;
  if(++timePulseCounter >= halfPeriod) {
    timePulseCounter = 0;
    timePulseOutput ^= 1;
  }

  if(command == RegisterHold || command == TimeRead) {
    outputBit = counter.bit(14);
  }
}

auto uPD4990A::tickSecond() -> void {
  auto bcdIncrement = [](n8& data) -> n8 {
    if((data & 0x0f) >= 0x09) {
      data = (data & 0xf0) + 0x10;
    } else {
      data++;
    }
    return data;
  };

  if(bcdIncrement(second()) < 0x60) return;
  second() = 0;

  if(bcdIncrement(minute()) < 0x60) return;
  minute() = 0;

  if(bcdIncrement(hour()) < 0x24) return;
  hour() = 0;

  setWeekday((getWeekday() + 1) % 7);

  n8 bcdDaysInMonth[12] = {0x31,0x28,0x31,0x30,0x31,0x30,0x31,0x31,0x30,0x31,0x30,0x31};
  u32 y = BCD::decode(year());
  if(!(y % 4)) bcdDaysInMonth[1] = 0x29;

  u32 m = getMonth();
  if(m >= 1 && m <= 12) {
    if(bcdIncrement(day()) <= bcdDaysInMonth[m - 1]) return;
  }
  day() = 0x01;

  m++;
  if(m <= 12) {
    setMonth(m);
    return;
  }
  setMonth(1);

  if(bcdIncrement(year()) > 0x99) year() = 0;
}

auto uPD4990A::dataOut() -> n1 {
  return outputBit;
}

auto uPD4990A::timePulse() -> n1 {
  return timePulseOutput;
}

auto uPD4990A::write(n1 din, n1 clock, n1 strobe) -> void {
  if(strobe && !strobePin) {
    command = commandRegister;

    if(command == TimeSet) {
      for(u32 index : range(6)) data[index] = shiftRegister[index];
    }

    if(command == TimeRead) {
      for(u32 index : range(6)) shiftRegister[index] = data[index];
      outputBit = counter.bit(14);
    }

    if(command == RegisterShift) {
      outputBit = shiftRegister[0] & 1;
    }

    if(command == RegisterHold) outputBit = counter.bit(14);
    if(command >= TimePulse64Hz && command <= TimePulse60s) {
      timePulseMode = command;
      timePulseCounter = 0;
      timePulseOutput = 0;
    }
  }
  strobePin = strobe;

  if(clock && !clockPin) {
    n1 cmdOut = commandRegister.bit(0);
    commandRegister >>= 1;
    commandRegister.bit(3) = din;

    if(command == RegisterShift) {
      for(u32 index : range(6)) {
        n1 nextBit = (index < 5) ? n1(shiftRegister[index + 1] & 1) : n1(cmdOut);
        shiftRegister[index] >>= 1;
        shiftRegister[index] |= (nextBit << 7);
      }
      outputBit = shiftRegister[0] & 1;
    }
  }
  clockPin = clock;
}

}
