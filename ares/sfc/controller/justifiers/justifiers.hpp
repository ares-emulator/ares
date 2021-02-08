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
  auto data() -> n2;
  auto latch(n1 data) -> void;

private:
  i32 cx1 = 256 / 2 - 16;
  i32 cy1 = 240 / 2;
  i32 cx2 = 256 / 2 + 16;
  i32 cy2 = 240 / 2;

  n1  active;  //0 = player 1; 1 = player 2
  n1  latched;
  n32 counter;
  n32 previous;
};
