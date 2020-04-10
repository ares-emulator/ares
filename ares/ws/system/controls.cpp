auto System::Controls::load(Node::Object parent) -> void {
  node = parent->append<Node::Object>("Controls");

  if(Model::WonderSwan() || Model::WonderSwanColor() || Model::SwanCrystal()) {
    y1     = node->append<Node::Button>("Y1");
    y2     = node->append<Node::Button>("Y2");
    y3     = node->append<Node::Button>("Y3");
    y4     = node->append<Node::Button>("Y4");
    x1     = node->append<Node::Button>("X1");
    x2     = node->append<Node::Button>("X2");
    x3     = node->append<Node::Button>("X3");
    x4     = node->append<Node::Button>("X4");
    b      = node->append<Node::Button>("B");
    a      = node->append<Node::Button>("A");
    start  = node->append<Node::Button>("Start");
    volume = node->append<Node::Button>("Volume");
  }

  if(Model::PocketChallengeV2()) {
    up     = node->append<Node::Button>("Up");
    down   = node->append<Node::Button>("Down");
    left   = node->append<Node::Button>("Left");
    right  = node->append<Node::Button>("Right");
    pass   = node->append<Node::Button>("Pass");
    circle = node->append<Node::Button>("Circle");
    clear  = node->append<Node::Button>("Clear");
    view   = node->append<Node::Button>("View");
    escape = node->append<Node::Button>("Escape");
  }

  power = node->append<Node::Button>("Power");
}

auto System::Controls::poll() -> void {
  if(Model::WonderSwan() || Model::WonderSwanColor() || Model::SwanCrystal()) {
    platform->input(y1);
    platform->input(y2);
    platform->input(y3);
    platform->input(y4);
    platform->input(x1);
    platform->input(x2);
    platform->input(x3);
    platform->input(x4);
    platform->input(b);
    platform->input(a);
    platform->input(start);

    if(y1->value() || y2->value() || y3->value() || y4->value()
    || x1->value() || x2->value() || x3->value() || x4->value()
    || b->value() || a->value() || start->value()
    ) {
      cpu.raise(CPU::Interrupt::Input);
    }

    bool volumeValue = volume->value();
    platform->input(volume);
    if(!volumeValue && volume->value()) {
      //lower volume by one step. 0 wraps to 3 here (uint2 type.)
      apu.r.masterVolume--;
      //ASWAN has three volume steps; SPHINX and SPHINX2 have four.
      if(SoC::ASWAN() && apu.r.masterVolume == 3) apu.r.masterVolume = 2;
      ppu.updateIcons();
    }
  }

  if(Model::PocketChallengeV2()) {
    platform->input(up);
    platform->input(down);
    platform->input(left);
    platform->input(right);
    platform->input(pass);
    platform->input(circle);
    platform->input(clear);
    platform->input(view);
    platform->input(escape);

    //the Y-axis acts as independent buttons.
    //the X-axis has a rocker, which prevents both keys from being pressed at the same time.
    if(!(left->value() & right->value())) {
      xHold = 0, leftLatch = left->value(), rightLatch = right->value();
    } else if(!xHold) {
      xHold = 1, swap(leftLatch, rightLatch);
    }

    if(up->value() || down->value() || leftLatch || rightLatch
    || pass->value() || circle->value() || clear->value()
    || view->value() || escape->value()
    ) {
      cpu.raise(CPU::Interrupt::Input);
    }
  }

  bool powerValue = power->value();
  platform->input(power);
  if(!powerValue && power->value()) {
    scheduler.exit(Event::Power);
  }
}
