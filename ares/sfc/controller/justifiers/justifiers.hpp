struct Justifiers : Controller, Thread {
  Node::Video::Sprite sprite1;
  Node::Video::Sprite sprite2;
  Node::Input::Axis x1;
  Node::Input::Axis y1;
  Node::Input::Button trigger1;
  Node::Input::Button start1;
  Node::Input::Axis x2;
  Node::Input::Axis y2;
  Node::Input::Button trigger2;
  Node::Input::Button start2;

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
