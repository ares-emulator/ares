EpsonRTC epsonrtc;
#include "memory.cpp"
#include "time.cpp"
#include "serialization.cpp"

auto EpsonRTC::load(Node::Object parent) -> void {
  rtc = parent->append<Node::Component::RealTimeClock>("RTC");
}

auto EpsonRTC::main() -> void {
  if(wait) { if(--wait == 0) ready = 1; }

  clocks++;
  if((clocks & ~0x00ff) == 0) roundSeconds();  //125 microseconds
  if((clocks & ~0x3fff) == 0) duty();  //1/128th second
  if((clocks & ~0x7fff) == 0) irq(0);  //1/64th second
  if(clocks == 0) {  //1 second
    seconds++;
    irq(1);
    if(seconds %   60 == 0) irq(2);  //1 minute
    if(seconds % 1440 == 0) irq(3), seconds = 0;  //1 hour
    tick();
  }

  Thread::step(1);
  Thread::synchronize(cpu);
}

auto EpsonRTC::initialize() -> void {
  secondlo = 0;
  secondhi = 0;
  batteryfailure = 1;

  minutelo = 0;
  minutehi = 0;
  resync = 0;

  hourlo = 0;
  hourhi = 0;
  meridian = 0;

  daylo = 0;
  dayhi = 0;
  dayram = 0;

  monthlo = 0;
  monthhi = 0;
  monthram = 0;

  yearlo = 0;
  yearhi = 0;

  weekday = 0;

  hold = 0;
  calendar = 0;
  irqflag = 0;
  roundseconds = 0;

  irqmask = 0;
  irqduty = 0;
  irqperiod = 0;

  pause = 0;
  stop = 0;
  atime = 0;
  test = 0;
}

auto EpsonRTC::unload() -> void {
  cpu.coprocessors.removeByValue(this);
  Thread::destroy();
}

auto EpsonRTC::power() -> void {
  Thread::create(32'768 * 64, {&EpsonRTC::main, this});
  cpu.coprocessors.append(this);

  clocks = 0;
  seconds = 0;

  chipselect = 0;
  state = State::Mode;
  offset = 0;
  wait = 0;
  ready = 0;
  holdtick = 0;
}

auto EpsonRTC::synchronize(n64 timestamp) -> void {
  time_t systime = timestamp;
  tm* timeinfo = localtime(&systime);

  u32 second = min(59, timeinfo->tm_sec);
  secondlo = second % 10;
  secondhi = second / 10;

  u32 minute = timeinfo->tm_min;
  minutelo = minute % 10;
  minutehi = minute / 10;

  u32 hour = timeinfo->tm_hour;
  if(atime) {
    hourlo = hour % 10;
    hourhi = hour / 10;
  } else {
    meridian = hour >= 12;
    hour %= 12;
    if(hour == 0) hour = 12;
    hourlo = hour % 10;
    hourhi = hour / 10;
  }

  u32 day = timeinfo->tm_mday;
  daylo = day % 10;
  dayhi = day / 10;

  u32 month = 1 + timeinfo->tm_mon;
  monthlo = month % 10;
  monthhi = month / 10;

  u32 year = timeinfo->tm_year % 100;
  yearlo = year % 10;
  yearhi = year / 10;

  weekday = timeinfo->tm_wday;

  resync = true;  //alert program that time has changed
}

auto EpsonRTC::read(n24 address, n8 data) -> n8 {
  cpu.synchronize(*this);
  address &= 3;

  if(address == 0) {
    return chipselect;
  }

  if(address == 1) {
    if(chipselect != 1) return 0;
    if(ready == 0) return 0;
    if(state == State::Write) return mdr;
    if(state != State::Read) return 0;
    ready = 0;
    wait = 8;
    return rtcRead(offset++);
  }

  if(address == 2) {
    return ready << 7;
  }

  return data;
}

auto EpsonRTC::write(n24 address, n8 data) -> void {
  cpu.synchronize(*this);
  address &= 3, data &= 15;

  if(address == 0) {
    chipselect = data;
    if(chipselect != 1) rtcReset();
    ready = 1;
  }

  if(address == 1) {
    if(chipselect != 1) return;
    if(ready == 0) return;

    if(state == State::Mode) {
      if(data != 0x03 && data != 0x0c) return;
      state = State::Seek;
      ready = 0;
      wait = 8;
      mdr = data;
    }

    else if(state == State::Seek) {
      if(mdr == 0x03) state = State::Write;
      if(mdr == 0x0c) state = State::Read;
      offset = data;
      ready = 0;
      wait = 8;
      mdr = data;
    }

    else if(state == State::Write) {
      rtcWrite(offset++, data);
      ready = 0;
      wait = 8;
      mdr = data;
    }
  }
}
