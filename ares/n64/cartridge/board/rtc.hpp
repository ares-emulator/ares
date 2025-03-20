struct RTC {
  Cartridge& self;
  RTC(Cartridge& self) : self(self) {}

  Memory::Writable ram;
  n1 present;
  n8 status;
  n3 writeLock;

  auto power(bool reset) -> void;
  auto run(bool run) -> void;
  auto running() -> bool;
  auto load() -> bool;
  auto save() -> bool;
  auto tick(int nsec=1) -> void;
  auto advance(int nsec) -> void;
  auto serialize(serializer& s) -> void;
  auto read(u2 block, n8 *data) -> void;
  auto write(u2 block, n8 *data) -> void;

  auto joybusComm(n8 send, n8 recv, n8 input[], n8 output[]) -> n1;
} rtc{this->cartridge};
