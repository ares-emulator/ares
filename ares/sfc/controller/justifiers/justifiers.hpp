struct Justifiers : Controller, Thread {
  Node::Sprite sprite1;
  Node::Sprite sprite2;
  Node::Axis x1;
  Node::Axis y1;
  Node::Button trigger1;
  Node::Button start1;
  Node::Axis x2;
  Node::Axis y2;
  Node::Button trigger2;
  Node::Button start2;

  Justifiers(Node::Port);
  ~Justifiers();

  auto main() -> void;
  auto data() -> uint2;
  auto latch(bool data) -> void;

private:
  int  cx1 = 256 / 2 - 16;
  int  cy1 = 240 / 2;
  int  cx2 = 256 / 2 + 16;
  int  cy2 = 240 / 2;

  bool active = 0;  //0 = player 1; 1 = player 2
  bool latched = 0;
  uint counter = 0;
  uint previous = 0;
};
