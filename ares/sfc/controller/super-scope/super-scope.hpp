struct SuperScope : Controller, Thread {
  Node::Sprite sprite;
  Node::Axis x;
  Node::Axis y;
  Node::Button trigger;
  Node::Button cursor;
  Node::Button turbo;
  Node::Button pause;

  SuperScope(Node::Port);
  ~SuperScope();

  auto main() -> void;
  auto data() -> uint2;
  auto latch(bool data) -> void;

private:
  bool latched = 0;
  uint counter = 0;

  int  cx = 256 / 2;
  int  cy = 240 / 2;
  bool triggerValue = 0;
  bool turboEdge = 0;
  bool pauseEdge = 0;

  bool offscreen = 0;
  bool turboOld = 0;
  bool triggerLock = 0;
  bool pauseLock = 0;
  uint previous = 0;
};
