struct EEPROM : M93LCx6 {
  enum : u32 {
    DataLo,
    DataHi,
    CommandLo,
    CommandHi,
    Status,
    Control = Status,
  };

  //eeprom.cpp
  auto power() -> void;
  virtual auto read(u32 port) -> n8;
  auto write(u32 port, n8 data) -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

protected:
  virtual auto canWrite(n16 command) -> bool;
  virtual auto padCommand(n16 command) -> n16;

  struct IO {
    n16 read;
    n16 write;
    n16 command;

    n1 ready;
    n1 readComplete;
    n4 control;
    n1 protect;
  } io;
};

struct InternalEEPROM : EEPROM {
  auto read(u32 port) -> n8 override;
  auto canWrite(n16 command) -> bool override;
  auto padCommand(n16 command) -> n16 override;
};
