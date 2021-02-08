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

  auto data() -> n2;
  auto latch(n1 data) -> void;

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
