struct SportsPad : Controller, Thread {
  Node::Input::Axis   x;
  Node::Input::Axis   y;
  Node::Input::Button one;
  Node::Input::Button two;

  SportsPad(Node::Port);
  ~SportsPad();

  auto main() -> void;
  auto read() -> n7 override;
  auto write(n4 data) -> void override;

private:
  // In theory values ranging from -128 to +127 are
  // possible, but on real hardware, when polling the
  // sports pad at 60 hz and giving it a really intense
  // work out, the maximum observed was +/- 60.
  s8 maxspeed = 60;

  n32 timeout;
  n4  status[4];
  n2  index = 0;
  n1  th = 1;
};
