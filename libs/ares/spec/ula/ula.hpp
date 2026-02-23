struct ULA : Thread {
  Node::Object node;
  Node::Video::Screen screen;
  Node::Audio::Stream stream;

  auto load(Node::Object) -> void;
  auto unload() -> void;

  auto main() -> void;
  auto step(uint clocks) -> void;
  auto frame() -> void;
  auto power() -> void;

  auto fetch(n16 address) -> n8;
  auto in(n16 port) -> n8;
  auto out(n8 data) -> void;

  auto activeDisplay() -> n1 { return vcounter >= screentop_start && vcounter < border_bottom_start && hcounter >= screenleft_start && hcounter < border_right_start; }
  auto floatingBus() -> n8 { return activeDisplay() ? busValue : (n8)0xff; }

  auto serialize(serializer&) -> void;

  auto color(n32 color) -> n64;

  struct IO {
    u3 borderColor;
    u1 mic;
    u1 ear;
  } io;

  n16 hcounter;
  n16 vcounter;
  n5 flashFrameCounter;
  n1 flashState;
  n8 busValue;

  uint border_top_start = 16;
  uint screentop_start = border_top_start + 48;
  uint border_bottom_start = screentop_start + 192;
  uint border_bottom_end = border_bottom_start + 56;
  uint border_left_start = 96;
  uint screenleft_start = border_left_start + 48;
  uint border_right_start = screenleft_start + 256;
  uint border_right_end = border_right_start + 48;
};

extern ULA ula;
