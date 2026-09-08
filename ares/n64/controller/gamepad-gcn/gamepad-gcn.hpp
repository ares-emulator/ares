//GameCube controller connected to a Nintendo 64 controller port.
//
//The N64 joybus is electrically compatible with the GameCube's, and homebrew
//built with libdragon speaks the GameCube controller protocol directly
//(see joypad.c: JOYPAD_STYLE_GCN). Commercial N64 titles only understand the
//N64 protocol and will not recognize this device.
struct GamepadGCN : Controller {
  Node::Input::Axis stickAxis;
  Node::Input::Axis cstickAxis;

  Node::Input::Axis x;
  Node::Input::Axis y;
  Node::Input::Axis cx;
  Node::Input::Axis cy;
  Node::Input::Axis lAnalog;
  Node::Input::Axis rAnalog;
  Node::Input::Button up;
  Node::Input::Button down;
  Node::Input::Button left;
  Node::Input::Button right;
  Node::Input::Button a;
  Node::Input::Button b;
  Node::Input::Button xButton;
  Node::Input::Button yButton;
  Node::Input::Button l;
  Node::Input::Button r;
  Node::Input::Button z;
  Node::Input::Button start;
  Node::Input::Rumble motor;

  GamepadGCN(Node::Port);
  ~GamepadGCN();
  auto rumble(bool enable) -> void;
  auto comm(n8 send, n8 recv, n8 input[], n8 output[]) -> n2 override;
  auto reset() -> void override;
  auto serialize(serializer&) -> void override;

  //current controller state, sampled by poll()
  struct State {
    n8 stickX;
    n8 stickY;
    n8 cstickX;
    n8 cstickY;
    n8 analogL;
    n8 analogR;
    n1 a, b, x, y, z, l, r, start;
    n1 up, down, left, right;
  } state;

  auto poll() -> void;
  auto writeButtons(n8 output[]) -> void;
  auto writeAnalogMode(n8 mode, n8 output[]) -> void;
  auto writeOrigin(n8 output[]) -> void;

  //origins are the neutral readings the console subtracts from every sample.
  //libdragon assumes JOYPAD_GCN_ORIGIN_INIT until it reads the real values
  //with the origin command, so both must agree.
  static constexpr u8 OriginStick   = 127;
  static constexpr u8 OriginCStick  = 127;
  static constexpr u8 OriginTrigger = 0;

  //ranges libdragon normalizes against (JOYPAD_RANGE_GCN_*)
  static constexpr f64 RangeStick   = 100.0;
  static constexpr f64 RangeCStick  =  76.0;
  static constexpr f64 RangeTrigger = 200.0;

  //fraction of trigger travel before the click switch engages, matching the
  //default Dolphin uses for the same physical control
  static constexpr f64 TriggerThreshold = 0.90;

  n1 originPending;  //set until the console has read the origins once
};
