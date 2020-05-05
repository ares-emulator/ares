//Reality Display Processor

struct RDP : Thread {
  Node::Component node;
  Node::Screen screen;

  //rdp.cpp
  auto load(Node::Object) -> void;
  auto unload() -> void;

  auto main() -> void;
  auto step(uint clocks) -> void;
  auto refresh() -> void;

  auto power() -> void;

  //io-command.cpp
  auto readCommand(u32 address) -> u32;
  auto writeCommand(u32 address, u32 data) -> void;

  //io-span.cpp
  auto readSpan(u32 address) -> u32;
  auto writeSpan(u32 address, u32 data) -> void;

  //serialization.cpp
  auto serialize(serializer&) -> void;

  u32 output[640 * 480];
  bool refreshed;

  struct IO {
     u32 vcounter;
    bool field;
  } io;
};

extern RDP rdp;
