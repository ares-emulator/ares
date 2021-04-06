struct Bus {
  Node::Object node;

  static constexpr inline u32 APU     = 1 << 0;
  static constexpr inline u32 VDPFIFO = 1 << 1;
  static constexpr inline u32 VDPDMA  = 1 << 2;

  auto acquired() const -> bool { return state.acquired; }
  auto acquire(u32 owner) -> void { state.acquired |=  owner; }
  auto release(u32 owner) -> void { state.acquired &= ~owner; }

  //inline.hpp
  auto read(n1 upper, n1 lower, n24 address, n16 data = 0) -> n16;
  auto write(n1 upper, n1 lower, n24 address, n16 data) -> void;
  auto waitRefreshExternal() -> void;
  auto waitRefreshRAM() -> void;

  //bus.cpp
  auto load(Node::Object) -> void;
  auto unload() -> void;
  auto power(bool reset) -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  struct State {
    u32 acquired = 0;
  } state;
};

extern Bus bus;
