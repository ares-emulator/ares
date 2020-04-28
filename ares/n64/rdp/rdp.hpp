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

  //serialization.cpp
  auto serialize(serializer&) -> void;

private:
  uint32 output[640 * 480];
};

extern RDP rdp;
