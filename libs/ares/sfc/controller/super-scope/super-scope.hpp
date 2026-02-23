struct SuperScope : Controller, Thread {
  Node::Video::Sprite sprite;
  Node::Input::Axis x;
  Node::Input::Axis y;
  Node::Input::Button trigger;
  Node::Input::Button cursor;
  Node::Input::Button turbo;
  Node::Input::Button pause;

  SuperScope(Node::Port);
  ~SuperScope();

  auto main() -> void;
  auto data() -> n2;
  auto latch(n1 data) -> void;

private:
  n1  latched;
  n32 counter;

  i32 cx = 256 / 2;
  i32 cy = 240 / 2;
  n1  triggerValue;
  n1  turboEdge;
  n1  pauseEdge;

  n1  offscreen;
  n1  turboOld;
  n1  triggerLock;
  n1  pauseLock;
  n32 previous;
};
