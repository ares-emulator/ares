struct DigitalGamepad : PeripheralDevice {
  Node::Input::Button up;
  Node::Input::Button down;
  Node::Input::Button left;
  Node::Input::Button right;
  Node::Input::Button cross;
  Node::Input::Button circle;
  Node::Input::Button square;
  Node::Input::Button triangle;
  Node::Input::Button l1;
  Node::Input::Button l2;
  Node::Input::Button r1;
  Node::Input::Button r2;
  Node::Input::Button select;
  Node::Input::Button start;

  DigitalGamepad(Node::Port);
  auto reset() -> void override;
  auto acknowledge() -> bool override;
  auto bus(u8 data) -> u8 override;

  enum class State : u32 {
    Idle,
    IDLower,
    IDUpper,
    DataLower,
    DataUpper,
    Release,
  } state = State::Idle;
};
