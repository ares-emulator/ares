struct TrakBall : Controller {
  Node::Input::Axis x;
  Node::Input::Axis y;

  //trak-ball.cpp
  TrakBall(Node::Port parent);

  auto poll() -> void override;
  auto axis(u32 index, bool powered) -> s16 override;

private:
  //trak-ball.cpp
  auto velocity(s16 motion) -> s16;

  static constexpr s32 MotionScale = 1024;
  s16 velocityX = 0;
  s16 velocityY = 0;
};
