struct Gamepad : Controller {
  Node::Input::Button up;
  Node::Input::Button down;
  Node::Input::Button left;
  Node::Input::Button right;
  Node::Input::Button l;
  Node::Input::Button r;
  Node::Input::Button one;
  Node::Input::Button two;
  Node::Input::Button three;
  Node::Input::Button four;
  Node::Input::Button five;
  Node::Input::Button six;
  Node::Input::Button seven;
  Node::Input::Button eight;
  Node::Input::Button nine;
  Node::Input::Button star;
  Node::Input::Button zero;
  Node::Input::Button pound;

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
