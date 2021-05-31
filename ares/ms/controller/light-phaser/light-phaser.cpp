LightPhaser::LightPhaser(Node::Port parent) {
  node = parent->append<Node::Peripheral>("Light Phaser");

  x       = node->append<Node::Input::Axis  >("X");
  y       = node->append<Node::Input::Axis  >("Y");
  trigger = node->append<Node::Input::Button>("Trigger");

  sprite = node->append<Node::Video::Sprite>("Crosshair");
  sprite->setImage(Resource::Sprite::SuperFamicom::CrosshairGreen);
  vdp.screen->attach(sprite);
}

LightPhaser::~LightPhaser() {
  vdp.screen->detach(sprite);
}

auto LightPhaser::read() -> n7 {
  sprite->setPosition(64, 64);
  sprite->setVisible(true);

  platform->input(trigger);

  n7 data;
  data.bit(0) = 1;
  data.bit(1) = 1;
  data.bit(2) = 1;
  data.bit(3) = 1;
  data.bit(4) = !trigger->value();
  data.bit(5) = 1;
  data.bit(6) = 1;
  return data;
}
