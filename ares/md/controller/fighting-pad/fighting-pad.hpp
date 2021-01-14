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
  auto main() -> void;
  auto readData() -> uint8 override;
  auto writeData(uint8 data) -> void override;

private:
  uint1 select = 1;
  uint1 latch;
  uint3 counter;
  uint32 timeout;

  bool yHold = 0;
  bool upLatch = 0;
  bool downLatch = 0;
  bool xHold = 0;
  bool leftLatch = 0;
  bool rightLatch = 0;
};
