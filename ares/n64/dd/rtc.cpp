auto DD::RTC::load() -> void {
  ram.allocate(0x10);
  if(auto fp = system.pak->read("time.rtc")) {
    ram.load(fp);
  }

  n64 check = 0;
  for(auto n : range(8)) check.byte(n) = ram.read<Byte>(n);
  if(!~check) return;  //new save file

  n64 timestamp = 0;
  for(auto n : range(8)) timestamp.byte(n) = ram.read<Byte>(8 + n);
  if(!~timestamp) return;  //new save file

  timestamp = time(0) - timestamp;
  while(timestamp--) tickSecond();
}

auto DD::RTC::reset() -> void {
  ram.reset();
}

auto DD::RTC::save() -> void {
  n64 timestamp = time(0);
  for(auto n : range(8)) ram.write<Byte>(8 + n, timestamp.byte(n));

  if(auto fp = system.pak->write("time.rtc")) {
    ram.save(fp);
  }
}

auto DD::RTC::serialize(serializer& s) -> void {
  s(ram);
}

auto DD::RTC::tick(u32 offset) -> void {
  u8 n = ram.read<Byte>(offset);
  if((++n & 0xf) > 9) n = (n & 0xf0) + 0x10;
  if((n & 0xf0) > 0x90) n = 0;
  ram.write<Byte>(offset, n);
}

auto DD::RTC::tickClock() -> void {
  tickSecond();
  queue.remove(Queue::DD_Clock_Tick);
  queue.insert(Queue::DD_Clock_Tick, 187'500'000);
}

auto DD::RTC::tickSecond() -> void {
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
  u32 daysInMonth[12] = {0x31, 0x28, 0x31, 0x30, 0x31, 0x30, 0x31, 0x31, 0x30, 0x31, 0x30, 0x31};
  if(ram.read<Byte>(0) && !(BCD::decode(ram.read<Byte>(0)) % 4)) daysInMonth[1]++;

  tick(2);
  if(ram.read<Byte>(2) <= daysInMonth[BCD::decode(ram.read<Byte>(1))-1]) return;
  ram.write<Byte>(2, 1);

  //month
  tick(1);
  if(ram.read<Byte>(1) < 0x12) return;
  ram.write<Byte>(1, 1);

  //year
  tick(0);
}
