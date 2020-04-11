Justifier::Justifier(Node::Port parent) {
  node = parent->append<Node::Peripheral>("Justifier");

  x       = node->append<Node::Axis  >("X");
  y       = node->append<Node::Axis  >("Y");
  trigger = node->append<Node::Button>("Trigger");
  start   = node->append<Node::Button>("Start");

  sprite = node->append<Node::Sprite>("Crosshair");
  sprite->setImage(Resource::Sprite::SuperFamicom::CrosshairGreen);
  ppu.screen->attach(sprite);

  Thread::create(system.cpuFrequency(), {&Justifier::main, this});
  cpu.peripherals.append(this);
}

Justifier::~Justifier() {
  cpu.peripherals.removeByValue(this);
  ppu.screen->detach(sprite);
}

auto Justifier::main() -> void {
  uint next = cpu.vcounter() * 1364 + cpu.hcounter();

  int px = active == 0 ? (int)x->value() : -1;
  int py = active == 0 ? (int)y->value() : -1;
  bool offscreen = px < 0 || py < 0 || px >= 256 || py >= ppu.vdisp();

  if(!offscreen) {
    uint target = py * 1364 + (px + 24) * 4;
    if(next >= target && previous < target) {
      //CRT raster detected, strobe iobit to latch counters
      iobit(0);
      iobit(1);
    }
  }

  if(next < previous) {
    platform->input(x);
    platform->input(y);
    int nx = x->value() + cx;
    int ny = y->value() + cy;
    cx = max(-16, min(256 + 16, nx));
    cy = max(-16, min(240 + 16, ny));
    sprite->setPosition(cx * 2 - 16, cy * 2 -16);
    sprite->setVisible(true);
  }

  previous = next;
  step(2);
  synchronize(cpu);
}

auto Justifier::data() -> uint2 {
  if(counter == 0) {
    platform->input(trigger);
    platform->input(start);
  }

  switch(counter++) {
  case  0: return 0;
  case  1: return 0;
  case  2: return 0;
  case  3: return 0;
  case  4: return 0;
  case  5: return 0;
  case  6: return 0;
  case  7: return 0;
  case  8: return 0;
  case  9: return 0;
  case 10: return 0;
  case 11: return 0;

  case 12: return 1;  //4-bit device signature
  case 13: return 1;
  case 14: return 1;
  case 15: return 0;

  case 16: return 0;
  case 17: return 1;
  case 18: return 0;
  case 19: return 1;
  case 20: return 0;
  case 21: return 1;
  case 22: return 0;
  case 23: return 1;

  case 24: return trigger->value();
  case 25: return 0;
  case 26: return start->value();
  case 27: return 0;
  case 28: return active;
  case 29: return 0;
  case 30: return 0;
  case 31: return 0;
  }

  if(counter > 32) counter = 32;
  return 1;
}

auto Justifier::latch(bool data) -> void {
  if(latched != data) {
    latched = data;
    counter = 0;
    if(!latched) active = !active;  //occurs even with only one Justifier
  }
}
