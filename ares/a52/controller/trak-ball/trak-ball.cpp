TrakBall::TrakBall(Node::Port parent) : Controller(parent, "Trak-Ball") {
  x = node->append<Node::Input::Axis>("X-Axis");
  y = node->append<Node::Input::Axis>("Y-Axis");
  appendControls();
}

auto TrakBall::poll() -> void {
  if(platform) {
    platform->input(x);
    platform->input(y);
  }
  velocityX = velocity(x->value());
  velocityY = velocity(y->value());
  Controller::poll();
}

auto TrakBall::velocity(s16 motion) -> s16 {
  // The service manual specifies a 900:600 maximum slope-change ratio for
  // left/up versus right/down, but not a CPU-visible count per optical pulse.
  // MotionScale is therefore an explicit host-to-controller approximation.
  s32 result = (s32)motion * MotionScale;
  if(result < 0) result = result * 3 / 2;
  return sclamp<16>(result);
}

auto TrakBall::axis(u32 index, bool powered) -> s16 {
  if(!powered) return 0;
  return index == 0 ? velocityX : velocityY;
}
