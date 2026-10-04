namespace ELF {

static constexpr u32 MaximumImageSize = 16_MiB;

struct BusQueue {
  static constexpr u32 Capacity = 16384;

  struct Transaction {
    u16 address = 0;
    u16 mask = 0x1fff;
    u8 data = 0;
    u64 timestamp = 0;
    bool yield = false;
  };

  auto reset() -> void;
  auto size() const -> u32 { return count; }
  auto empty() const -> bool { return !count; }
  auto setTimestamp(u64 value) -> BusQueue& { timestamp = value; return *this; }
  auto setNextAddress(u16 value) -> BusQueue& { nextAddress = value & 0x1fff; return *this; }
  auto getNextAddress() const -> u16 { return nextAddress; }
  auto inject(u8 data) -> bool;
  auto injectAt(u16 address, u8 data) -> bool;
  auto stuff(u16 address, u8 data) -> bool;
  auto release(u16 address, u16 mask = 0x1fff) -> bool;
  auto consume(u16 address, u64 timestamp, Transaction& result) -> bool;
  auto serialize(serializer&) -> void;

private:
  auto push(Transaction) -> bool;

  Transaction queue[Capacity] = {};
  u32 head = 0;
  u32 count = 0;
  u16 nextAddress = 0;
  u64 timestamp = 0;
};

enum : u32 {
  SHT_NULL = 0,
  SHT_PROGBITS = 1,
  SHT_SYMTAB = 2,
  SHT_STRTAB = 3,
  SHT_RELA = 4,
  SHT_NOBITS = 8,
  SHT_REL = 9,
  SHT_INIT_ARRAY = 14,
  SHT_PREINIT_ARRAY = 16,
  SHF_WRITE = 1,
  SHF_ALLOC = 2,
  SHF_EXECINSTR = 4,
  SHN_UNDEF = 0,
  SHN_ABS = 0xfff1,
  R_ARM_ABS32 = 2,
  R_ARM_REL32 = 3,
  R_ARM_THM_CALL = 10,
  R_ARM_THM_JUMP24 = 30,
  R_ARM_TARGET1 = 38,
};

struct Section {
  string name;
  u32 nameOffset = 0;
  u32 type = 0;
  u32 flags = 0;
  u32 address = 0;
  u32 offset = 0;
  u32 size = 0;
  u32 link = 0;
  u32 info = 0;
  u32 alignment = 1;
  u32 entrySize = 0;
};

struct Symbol {
  string name;
  u32 nameOffset = 0;
  u32 value = 0;
  u32 size = 0;
  u8 bind = 0;
  u8 type = 0;
  u8 visibility = 0;
  u16 section = 0;
};

struct Relocation {
  u32 offset = 0;
  u32 symbol = 0;
  u8 type = 0;
  i32 addend = 0;
  bool explicitAddend = false;
};

struct Object {
  auto parse(std::span<const u8> source) -> bool;
  auto valid() const -> bool { return parsed; }
  auto error() const -> const string& { return failure; }
  auto bytes(u32 offset, u32 size) const -> std::span<const u8>;

  std::vector<u8> image;
  std::vector<Section> sections;
  std::vector<Symbol> symbols;
  std::vector<std::vector<Relocation>> relocations;
  string failure;
  bool parsed = false;
};

struct Linker {
  enum class Segment : u32 { Text, Data, Rodata };

  struct External {
    string name;
    u32 value = 0;
  };

  struct Placement {
    bool allocated = false;
    Segment segment = Segment::Text;
    u32 offset = 0;
  };

  struct Result {
    std::vector<u8> text;
    std::vector<u8> data;
    std::vector<u8> rodata;
    std::vector<u32> preinit;
    std::vector<u32> init;
    std::vector<Placement> placements;
    std::vector<u32> relocatedSymbols;
    u32 entrypoint = 0;
  };

  auto link(const Object&, std::span<const External>) -> bool;
  auto error() const -> const string& { return failure; }
  auto result() const -> const Result& { return linked; }

  static constexpr u32 TextBase = 0x0010'0000;
  static constexpr u32 TextCapacity = 1_MiB;
  static constexpr u32 DataBase = 0x0020'0000;
  static constexpr u32 DataCapacity = 512_KiB;
  static constexpr u32 RodataBase = 0x0030'0000;
  static constexpr u32 RodataCapacity = 512_KiB;

private:
  Result linked;
  string failure;
};

struct Memory {
  static constexpr u32 StubBase = 0x0000'1000;
  static constexpr u32 StackBase = 0x0000'4000;
  static constexpr u32 StackSize = 32_KiB;
  static constexpr u32 TablesBase = 0x0040'0000;
  static constexpr u32 TablesSize = 4_KiB;
  static constexpr u32 PeripheralBase = 0xf000'0000;

  auto power(const Linker::Result&) -> void;
  auto unload() -> void;
  auto get(u32 mode, n32 address, n32& value) -> ARMv6M::Fault;
  auto set(u32 mode, n32 address, n32 value) -> ARMv6M::Fault;
  auto writeWord(u32 address, u32 value) -> bool;
  auto serialize(serializer&) -> void;

  std::vector<u8> stack;
  std::vector<u8> text;
  std::vector<u8> data;
  std::vector<u8> rodata;
  std::vector<u8> tables;
  u32 peripheral[4] = {};
};

struct VCSLib {
  static constexpr u32 StubCount = 38;

  auto power() -> void;
  auto access(n16 address, n8 value, Memory&) -> n8;
  auto execute(u32 address, ARMv6M&, Memory&) -> ARMv6M::Fault;
  auto stalled() const -> bool;
  auto serialize(serializer&) -> void;

  BusQueue queue;
  bool busDriven = false;
  u8 busValue = 0;
  u16 currentAddress = 0;
  u8 currentData = 0;
  bool waiting = false;
  u16 waitingAddress = 0;
  u8 stuffMaskA = 0;
  u8 stuffMaskX = 0;
  u8 stuffMaskY = 0;
  u32 randomState = 1;
  u64 busAccessCounter = 0;

private:
  auto bootstrap() -> void;
  auto copyOverblank() -> void;
  auto write5(u16 address, u8 value) -> void;
};

struct Runtime : ARMv6M, Thread {
  enum class Stage : u32 { Preinit, Init, Main };
  enum class Failure : u32 { None, Instruction, MainReturn, PreinitEntry, InitEntry, MainEntry };

  auto power(const Linker::Result&, bool ntsc) -> void;
  auto unload() -> void;
  auto main() -> void;
  auto access(n16 address, n8 value) -> n8;
  auto invoke(u32 address, u32 stack, u32 argument0) -> Fault;
  auto startNext() -> void;
  auto fail(Failure, u32 address = 0) -> void;
  auto error() const -> string;
  auto faulted() const -> bool { return failure != Failure::None; }

  auto step(u32 clocks) -> void override;
  auto get(u32 mode, n32 address, n32& value) -> Fault override;
  auto getDebugger(u32 mode, n32 address, n32& value) -> Fault override;
  auto set(u32 mode, n32 address, n32 value) -> Fault override;
  auto serialize(serializer&) -> void;

  Memory memory;
  VCSLib vcs;
  std::vector<u32> preinit;
  std::vector<u32> init;
  u32 entrypoint = 0;
  u32 systemType = 0;
  Stage stage = Stage::Preinit;
  u32 initIndex = 0;
  bool ready = false;
  Failure failure = Failure::None;
  u32 failureAddress = 0;
};

auto externalSymbols(bool ntsc) -> std::vector<Linker::External>;

}
