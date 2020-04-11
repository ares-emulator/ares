struct FightingPad : Controller, Thread {
  Node::Button up;
  Node::Button down;
  Node::Button left;
  Node::Button right;
  Node::Button a;
  Node::Button b;
  Node::Button c;
  Node::Button x;
  Node::Button y;
  Node::Button z;
  Node::Button mode;
  Node::Button start;

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
