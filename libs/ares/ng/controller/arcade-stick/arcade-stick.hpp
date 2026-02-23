struct ArcadeStick : Controller {
  Node::Input::Button up;
  Node::Input::Button down;
  Node::Input::Button left;
  Node::Input::Button right;
  Node::Input::Button a;
  Node::Input::Button b;
  Node::Input::Button c;
  Node::Input::Button d;
  Node::Input::Button select;
  Node::Input::Button start;

  ArcadeStick(Node::Port);

  auto readButtons() -> n8 override;
  auto readControls() -> n2 override;

private:
  b1 yHold;
  b1 upLatch;
  b1 downLatch;
  b1 xHold;
  b1 leftLatch;
  b1 rightLatch;
};
