//Epson RTC-4513 Real-Time Clock

struct EpsonRTC : Thread {
  Node::Component::RealTimeClock rtc;
  auto load(Node::Object) -> void;

  using Thread::synchronize;

  auto main() -> void;

  auto initialize() -> void;
  auto unload() -> void;
  auto power() -> void;
  auto synchronize(n64 timestamp) -> void;

  auto read(n24 address, n8 data) -> n8;
  auto write(n24 address, n8 data) -> void;

  auto serialize(serializer&) -> void;

  n21 clocks;
  n32 seconds;

  n2  chipselect;
  enum class State : u32 { Mode, Seek, Read, Write } state;
  n4  mdr;
  n4  offset;
  u32 wait;
  n1  ready;
  n1  holdtick;

  n4  secondlo;
  n3  secondhi;
  n1  batteryfailure;

  n4  minutelo;
  n3  minutehi;
  n1  resync;

  n4  hourlo;
  n2  hourhi;
  n1  meridian;

  n4  daylo;
  n2  dayhi;
  n1  dayram;

  n4  monthlo;
  n1  monthhi;
  n2  monthram;

  n4  yearlo;
  n4  yearhi;

  n3  weekday;

  n1  hold;
  n1  calendar;
  n1  irqflag;
  n1  roundseconds;

  n1  irqmask;
  n1  irqduty;
  n2  irqperiod;

  n1  pause;
  n1  stop;
  n1  atime;  //astronomical time (24-hour mode)
  n1  test;

  //memory.cpp
  auto rtcReset() -> void;
  auto rtcRead(n4 address) -> n4;
  auto rtcWrite(n4 address, n4 data) -> void;

  auto load(const n8* data) -> void;
  auto save(n8* data) -> void;

  //time.cpp
  auto irq(n2 period) -> void;
  auto duty() -> void;
  auto roundSeconds() -> void;
  auto tick() -> void;

  auto tickSecond() -> void;
  auto tickMinute() -> void;
  auto tickHour() -> void;
  auto tickDay() -> void;
  auto tickMonth() -> void;
  auto tickYear() -> void;
};

extern EpsonRTC epsonrtc;
