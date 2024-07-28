struct SFC_Gamepad : Controller {
  Node::Input::Button up;
  Node::Input::Button down;
  Node::Input::Button left;
  Node::Input::Button right;
  Node::Input::Button b;
  Node::Input::Button a;
  Node::Input::Button y;
  Node::Input::Button x;
  Node::Input::Button l;
  Node::Input::Button r;
  Node::Input::Button select;
  Node::Input::Button start;

  SFC_Gamepad(Node::Port);

  auto data() -> n3 override;
  auto latch(n1 data) -> void override;
  auto serialize(serializer&) -> void override;

private:
  n1 latched;
  n8 counter;

  n1 yHold;
  n1 upLatch;
  n1 downLatch;
  n1 xHold;
  n1 leftLatch;
  n1 rightLatch;
};
