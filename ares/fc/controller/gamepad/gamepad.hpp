struct Gamepad : Controller {
  Node::Button up;
  Node::Button down;
  Node::Button left;
  Node::Button right;
  Node::Button b;
  Node::Button a;
  Node::Button select;
  Node::Button start;

  Gamepad(Node::Port);
  auto data() -> uint3 override;
  auto latch(uint1 data) -> void override;

private:
  bool latched = 0;
  uint counter = 0;

  bool yHold = 0;
  bool upLatch = 0;
  bool downLatch = 0;
  bool xHold = 0;
  bool leftLatch = 0;
  bool rightLatch = 0;
};
