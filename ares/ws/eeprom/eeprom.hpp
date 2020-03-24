struct EEPROM : M93LCx6 {
  enum : uint {
    DataLo,
    DataHi,
    AddressLo,
    AddressHi,
    Status,
    Command = Status,
  };

  //eeprom.cpp
  auto power() -> void;
  auto read(uint port) -> uint8;
  auto write(uint port, uint8 data) -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

private:
  struct Registers {
    uint16 data;
    uint16 address;

    //note: timing is not yet emulated; ready bits always remain set.
     uint1 readReady = 1;
     uint1 writeReady = 1;
     uint1 eraseReady = 1;
     uint1 resetReady = 1;
     uint1 readPending;
     uint1 writePending;
     uint1 erasePending;
     uint1 resetPending;
  } r;
};
