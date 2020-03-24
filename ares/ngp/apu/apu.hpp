struct APU : Z80, Z80::Bus, Thread {
  Node::Component node;
  Node::Instruction eventInstruction;
  Node::Notification eventInterrupt;
  Memory::Writable<uint8> ram;

  inline auto synchronizing() const -> bool override { return scheduler.synchronizing(); }

  //apu.cpp
  auto load(Node::Object, Node::Object) -> void;
  auto save() -> void;
  auto unload() -> void;

  auto main() -> void;
  auto step(uint clocks) -> void override;
  auto power() -> void;
  auto enable() -> void;
  auto disable() -> void;

  //memory.cpp
  auto read(uint16 address) -> uint8 override;
  auto write(uint16 address, uint8 data) -> void override;

  auto in(uint16 address) -> uint8 override;
  auto out(uint16 address, uint8 data) -> void override;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  struct NMI {
    uint1 line;
  } nmi;

  struct IRQ {
    uint1 line;
  } irq;

  struct Port {
    uint8 data;
  } port;

  struct IO {
    uint1 enable;
  } io;
};

extern APU apu;
