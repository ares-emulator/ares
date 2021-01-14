struct NTTDataKeypad : Controller {
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
  Node::Input::Button back;
  Node::Input::Button next;
  Node::Input::Button one;
  Node::Input::Button two;
  Node::Input::Button three;
  Node::Input::Button four;
  Node::Input::Button five;
  Node::Input::Button six;
  Node::Input::Button seven;
  Node::Input::Button eight;
  Node::Input::Button nine;
  Node::Input::Button zero;
  Node::Input::Button star;
  Node::Input::Button clear;
  Node::Input::Button pound;
  Node::Input::Button point;
  Node::Input::Button end;

  NTTDataKeypad(Node::Port);

  auto data() -> uint2;
  auto latch(bool data) -> void;

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
