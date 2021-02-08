struct HitachiDSP : HG51B, Thread {
  Node::Object node;
  ReadableMemory rom;
  WritableMemory ram;

  struct Debugger {
    //debugger.cpp
    auto load(Node::Object) -> void;
    auto instruction() -> void;

    struct Tracer {
      Node::Debugger::Tracer::Instruction instruction;  //todo: HG51B needs to notify HitachiDSP when instructiosn are executed
    } tracer;
  } debugger;

  //hitachidsp.cpp
  auto load(Node::Object) -> void;
  auto unload() -> void;

  auto step(u32 clocks) -> void override;
  auto halt() -> void override;

  auto power() -> void;

  auto isROM(n24 address) -> bool override;
  auto isRAM(n24 address) -> bool override;

  //HG51B read/write
  auto read(n24 address) -> n8 override;
  auto write(n24 address, n8 data) -> void override;

  //CPU ROM read/write
  auto addressROM(n24 address) const -> maybe<n24>;
  auto readROM(n24 address, n8 data = 0) -> n8;
  auto writeROM(n24 address, n8 data) -> void;

  //CPU RAM read/write
  auto addressRAM(n24 address) const -> maybe<n24>;
  auto readRAM(n24 address, n8 data = 0) -> n8;
  auto writeRAM(n24 address, n8 data) -> void;

  //HG51B data RAM read/write
  auto addressDRAM(n24 address) const -> maybe<n24>;
  auto readDRAM(n24 address, n8 data = 0) -> n8;
  auto writeDRAM(n24 address, n8 data) -> void;

  //CPU IO read/write
  auto addressIO(n24 address) const -> maybe<n24>;
  auto readIO(n24 address, n8 data = 0) -> n8;
  auto writeIO(n24 address, n8 data) -> void;

  auto firmware() const -> vector<n8>;
  auto serialize(serializer&) -> void;

  n32 Frequency;
  n32 Roms;
  n1  Mapping;
};

extern HitachiDSP hitachidsp;
