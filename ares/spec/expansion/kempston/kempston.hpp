struct Kempston : Expansion {
  Node::Input::Button up;
  Node::Input::Button down;
  Node::Input::Button left;
  Node::Input::Button right;
  Node::Input::Button fire;

  Kempston(Node::Port parent);

  auto romcs() -> bool { return false; }
  auto mapped(n16 address, bool io) -> bool { return io && (n8)address == 0x1f; }

  auto read(n16 address) -> n8 { return 0xff; }
  auto write(n16 address, n8 data) -> void {}

  auto in(n16 address) -> n8;
  auto out(n16 address, n8 data) -> void {}
};
