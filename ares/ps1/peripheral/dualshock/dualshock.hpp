struct DualShock : PeripheralDevice {
  Node::Input::Axis lx;
  Node::Input::Axis ly;
  Node::Input::Axis rx;
  Node::Input::Axis ry;
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
  Node::Input::Button l3;
  Node::Input::Button r1;
  Node::Input::Button r2;
  Node::Input::Button r3;
  Node::Input::Button select;
  Node::Input::Button start;
  Node::Input::Button mode;
  Node::Input::Rumble rumble;

  DualShock(Node::Port);
  auto reset() -> void override;
  auto acknowledge() -> bool override;
  auto bus(u8 data) -> u8 override;
  auto invalid(u8 data) -> u8;
  auto active() -> bool override;

  vector<u8> readPad();

  enum class State : u32 {
    Idle,
    IDLower,
    IDUpper,
    DataLower,
    DataUpper,
    AnalogDataLeftStickUpper,
    AnalogDataLeftStickLower,
    AnalogDataRightStickUpper,
    AnalogDataRightStickLower,
  } state = State::Idle;

  n1 analogMode;
  n1 newRumbleMode;
  n1 configMode;
  n8 command;
  i8 commandStep;
  n8 rumbleConfig[6];

  vector<u8> outputData;
  vector<n8> inputData;

  bool _active = false;
};
