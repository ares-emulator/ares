struct FightingPad : Controller, Thread {
  Node::Input::Button up;
  Node::Input::Button down;
  Node::Input::Button left;
  Node::Input::Button right;
  Node::Input::Button a;
  Node::Input::Button b;
  Node::Input::Button c;
  Node::Input::Button x;
  Node::Input::Button y;
  Node::Input::Button z;
  Node::Input::Button mode;
  Node::Input::Button start;

  FightingPad(Node::Port);
  ~FightingPad();
  auto main() -> void;
  auto poll() -> void override;
  auto readData() -> Data override;
  auto writeData(n8 data) -> void override;

private:
  n1  select = 1;
  n3  counter;
  n32 timeout;

  b1  yHold;
  b1  upLatch;
  b1  downLatch;
  b1  xHold;
  b1  leftLatch;
  b1  rightLatch;
};
