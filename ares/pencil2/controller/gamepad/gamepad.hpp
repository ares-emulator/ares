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

  auto read() -> n8 override;
  auto write(n8 data) -> void override;

  n1 select;

private:
  b1 yHold;
  b1 upLatch;
  b1 downLatch;
  b1 xHold;
  b1 leftLatch;
  b1 rightLatch;
};
