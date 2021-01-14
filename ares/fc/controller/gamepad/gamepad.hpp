struct Gamepad : Controller {
  Node::Input::Button up;
  Node::Input::Button down;
  Node::Input::Button left;
  Node::Input::Button right;
  Node::Input::Button b;
  Node::Input::Button a;
  Node::Input::Button select;
  Node::Input::Button start;

  Gamepad(Node::Port);
  auto data() -> n3 override;
  auto latch(n1 data) -> void override;

private:
  bool latched = 0;
  u32  counter = 0;

  bool yHold = 0;
  bool upLatch = 0;
  bool downLatch = 0;
  bool xHold = 0;
  bool leftLatch = 0;
  bool rightLatch = 0;
};
