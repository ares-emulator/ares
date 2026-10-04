struct StandardController : Controller {
  Node::Input::Axis x;
  Node::Input::Axis y;

  //standard.cpp
  StandardController(Node::Port parent);

  auto axis(u32 index, bool powered) -> s16 override;
};
