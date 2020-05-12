//Reality Display Processor

struct RDP : Thread {
  Node::Component node;

  //rdp.cpp
  auto load(Node::Object) -> void;
  auto unload() -> void;

  auto main() -> void;
  auto step(uint clocks) -> void;
  auto power() -> void;

  //io-command.cpp
  auto readCommand(u32 address) -> u32;
  auto writeCommand(u32 address, u32 data) -> void;

  //io-span.cpp
  auto readSpan(u32 address) -> u32;
  auto writeSpan(u32 address, u32 data) -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;
};

extern RDP rdp;
