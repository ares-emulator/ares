struct Justifier : Controller, Thread {
  Node::Video::Sprite sprite;
  Node::Input::Axis x;
  Node::Input::Axis y;
  Node::Input::Button trigger;
  Node::Input::Button start;

  Justifier(Node::Port);
  ~Justifier();

  auto main() -> void;
  auto data() -> n2;
  auto latch(n1 data) -> void;

private:
  i32 cx = 256 / 2;
  i32 cy = 240 / 2;

  n1  active;  //0 = player 1; 1 = player 2 (disconnected)
  n32 latched;
  n32 counter;
  n32 previous;
};
