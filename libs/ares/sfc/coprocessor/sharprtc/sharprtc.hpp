struct SharpRTC : Thread {
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

  enum class State : u32 { Ready, Command, Read, Write } state;
  s32 index;

  u32 second;
  u32 minute;
  u32 hour;
  u32 day;
  u32 month;
  u32 year;
  u32 weekday;

  //memory.cpp
  auto rtcRead(n4 address) -> n4;
  auto rtcWrite(n4 address, n4 data) -> void;

  auto load(const n8* data) -> void;
  auto save(n8* data) -> void;

  //time.cpp
  static const u32 daysInMonth[12];
  auto tickSecond() -> void;
  auto tickMinute() -> void;
  auto tickHour() -> void;
  auto tickDay() -> void;
  auto tickMonth() -> void;
  auto tickYear() -> void;

  auto calculateWeekday(u32 year, u32 month, u32 day) -> u32;
};

extern SharpRTC sharprtc;
