XG1LightGun::XG1LightGun(Node::Port parent) {
  node = parent->append<Node::Peripheral>("XG-1 Light Gun");

  x = node->append<Node::Input::Axis>("X");
  y = node->append<Node::Input::Axis>("Y");
  trigger = node->append<Node::Input::Button>("Trigger");

  targetY = video.displayHeight() / 2;
  sprite = node->append<Node::Video::Sprite>("Crosshair");
  sprite->setImage(Resource::Sprite::Famicom::Crosshair);
  video.screen->attach(sprite);
  updateCrosshair();
}

XG1LightGun::~XG1LightGun() {
  if(video.screen) video.screen->detach(sprite);
}

auto XG1LightGun::poll() -> void {
  platform->input(x);
  platform->input(y);

  targetX = std::clamp<s64>(targetX + x->value(), -TargetMargin, 160 + TargetMargin - 1);
  targetY = std::clamp<s64>(targetY + y->value(), -TargetMargin,
    video.displayHeight() + TargetMargin - 1);
  updateCrosshair();
}

auto XG1LightGun::read() -> n8 {
  platform->input(trigger);

  n8 data = 0xff;
  data.bit(0) = !trigger->value();
  data.bit(4) = !lightDetected();
  return data;
}

auto XG1LightGun::serialize(serializer& s) -> void {
  s(targetX);
  s(targetY);
  if(s.reading()) updateCrosshair();
}

auto XG1LightGun::lightDetected() const -> bool {
  auto beamX = tia.displayPosition() - 68 + HorizontalOffset;
  if(beamX < 0) beamX += HorizontalClocks;
  auto beamY = video.y() + VerticalOffset;
  auto deltaX = beamX - targetX;
  auto deltaY = beamY - targetY;
  return deltaX >= 0 && deltaX < DetectionWidth && deltaY >= 0;
}

auto XG1LightGun::updateCrosshair() -> void {
  sprite->setPosition(targetX + VideoLeftMargin - CrosshairRadius, targetY - CrosshairRadius);
  sprite->setVisible(targetX >= 0 && targetX < 160 && targetY >= 0 && targetY < video.displayHeight());
}
