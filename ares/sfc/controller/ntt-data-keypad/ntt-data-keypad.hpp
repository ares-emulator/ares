struct NTTDataKeypad : Controller {
  Node::Button up;
  Node::Button down;
  Node::Button left;
  Node::Button right;
  Node::Button b;
  Node::Button a;
  Node::Button y;
  Node::Button x;
  Node::Button l;
  Node::Button r;
  Node::Button back;
  Node::Button next;
  Node::Button one;
  Node::Button two;
  Node::Button three;
  Node::Button four;
  Node::Button five;
  Node::Button six;
  Node::Button seven;
  Node::Button eight;
  Node::Button nine;
  Node::Button zero;
  Node::Button star;
  Node::Button clear;
  Node::Button pound;
  Node::Button point;
  Node::Button end;

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
