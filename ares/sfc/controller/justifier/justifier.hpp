struct Justifier : Controller, Thread {
  Node::Sprite sprite;
  Node::Axis x;
  Node::Axis y;
  Node::Button trigger;
  Node::Button start;

  Justifier(Node::Port);
  ~Justifier();

  auto main() -> void;
  auto data() -> uint2;
  auto latch(bool data) -> void;

private:
  int  cx = 256 / 2;
  int  cy = 240 / 2;

  bool active = 0;  //0 = player 1; 1 = player 2 (disconnected)
  uint latched = 0;
  uint counter = 0;
  uint previous = 0;
};
