struct EEPROM : M93LCx6 {
  enum : u32 {
    DataLo,
    DataHi,
    AddressLo,
    AddressHi,
    Status,
    Command = Status,
  };

  //eeprom.cpp
  auto power() -> void;
  auto read(u32 port) -> n8;
  auto write(u32 port, n8 data) -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

private:
  struct IO {
    n16 data;
    n16 address;

    //note: timing is not yet emulated; ready bits always remain set.
    n1 readReady = 1;
    n1 writeReady = 1;
    n1 eraseReady = 1;
    n1 resetReady = 1;
    n1 readPending;
    n1 writePending;
    n1 erasePending;
    n1 resetPending;
  } io;
};
