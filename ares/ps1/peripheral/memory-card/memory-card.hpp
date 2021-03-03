struct MemoryCard : PeripheralDevice {
  VFS::Pak pak;
  Memory::Writable memory;

  MemoryCard(Node::Port);
  ~MemoryCard();
  auto reset() -> void override;
  auto acknowledge() -> bool override;
  auto bus(u8 data) -> u8 override;

  auto identify(u8 data) -> u8;
  auto read(u8 data) -> u8;
  auto write(u8 data) -> u8;

  enum class State : u32 {
    Idle,
    Select,
    Active,
  } state = State::Idle;

  enum class Command : u32 {
    None,
    Identify,
    Read,
    Write,
  } command = Command::None;

  struct Flag {
    u8 value;
    BitField<8,2> error{&value};
    BitField<8,3> fresh{&value};
    BitField<8,4> unknown{&value};
  } flag;

  u8  phase = 0;
  u8  checksum = 0;
  u8  response = 0;
  u16 address = 0;
};
