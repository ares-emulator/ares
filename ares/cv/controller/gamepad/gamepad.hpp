struct Gamepad : Controller {
  Node::Button up;
  Node::Button down;
  Node::Button left;
  Node::Button right;
  Node::Button l;
  Node::Button r;
  Node::Button one;
  Node::Button two;
  Node::Button three;
  Node::Button four;
  Node::Button five;
  Node::Button six;
  Node::Button seven;
  Node::Button eight;
  Node::Button nine;
  Node::Button star;
  Node::Button zero;
  Node::Button pound;

  Gamepad(Node::Port);

  auto read() -> uint8 override;
  auto write(uint8 data) -> void override;

  uint1 select;

private:
  bool yHold = 0;
  bool upLatch = 0;
  bool downLatch = 0;
  bool xHold = 0;
  bool leftLatch = 0;
  bool rightLatch = 0;
};
