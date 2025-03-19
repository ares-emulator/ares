struct Gamepad : Controller {
  Node::Port port;
  Node::Peripheral slot;
  VFS::Pak pak;
  u8 bank;
  Memory::Writable ram;  //Toshiba TC55257DFL-85V
  Node::Input::Rumble motor;

  Node::Input::Axis x;
  Node::Input::Axis y;
  Node::Input::Button up;
  Node::Input::Button down;
  Node::Input::Button left;
  Node::Input::Button right;
  Node::Input::Button b;
  Node::Input::Button a;
  Node::Input::Button cameraUp;
  Node::Input::Button cameraDown;
  Node::Input::Button cameraLeft;
  Node::Input::Button cameraRight;
  Node::Input::Button l;
  Node::Input::Button r;
  Node::Input::Button z;
  Node::Input::Button start;

  Gamepad(Node::Port);
  ~Gamepad();
  auto save() -> void override;
  auto allocate(string name) -> Node::Peripheral;
  auto connect() -> void;
  auto disconnect() -> void;
  auto rumble(bool enable) -> void;
  auto comm(n8 send, n8 recv, n8 input[], n8 output[]) -> n2 override;
  auto read() -> n32 override;
  auto getInodeChecksum(u8 bank) -> u8;
  auto formatControllerPak() -> void;
  auto serialize(serializer&) -> void override;

  struct TransferPak {
    auto load(Node::Object parent) -> void;
    auto unload() -> void;
    auto save() -> void;
    auto read(u16 address) -> u8;
    auto write(u16 address, u8 data) -> void;

    n1 pakEnable;
    n1 cartEnable;
    n2 resetState;
    n2 addressBank;
  } transferPak;
};
