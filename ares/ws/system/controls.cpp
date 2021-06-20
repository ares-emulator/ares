auto System::Controls::load(Node::Object parent) -> void {
  node = parent->append<Node::Object>("Controls");

  if(Model::WonderSwan() || Model::WonderSwanColor() || Model::SwanCrystal()) {
    y1     = node->append<Node::Input::Button>("Y1");
    y2     = node->append<Node::Input::Button>("Y2");
    y3     = node->append<Node::Input::Button>("Y3");
    y4     = node->append<Node::Input::Button>("Y4");
    x1     = node->append<Node::Input::Button>("X1");
    x2     = node->append<Node::Input::Button>("X2");
    x3     = node->append<Node::Input::Button>("X3");
    x4     = node->append<Node::Input::Button>("X4");
    b      = node->append<Node::Input::Button>("B");
    a      = node->append<Node::Input::Button>("A");
    start  = node->append<Node::Input::Button>("Start");
    volume = node->append<Node::Input::Button>("Volume");
  }

  if(Model::PocketChallengeV2()) {
    up     = node->append<Node::Input::Button>("Up");
    down   = node->append<Node::Input::Button>("Down");
    left   = node->append<Node::Input::Button>("Left");
    right  = node->append<Node::Input::Button>("Right");
    pass   = node->append<Node::Input::Button>("Pass");
    circle = node->append<Node::Input::Button>("Circle");
    clear  = node->append<Node::Input::Button>("Clear");
    view   = node->append<Node::Input::Button>("View");
    escape = node->append<Node::Input::Button>("Escape");
  }

  power = node->append<Node::Input::Button>("Power");
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
      //lower volume by one step. 0 wraps to 3 here (n2 type.)
      apu.io.masterVolume--;
      //ASWAN has three volume steps; SPHINX and SPHINX2 have four.
      if(SoC::ASWAN() && apu.io.masterVolume == 3) apu.io.masterVolume = 2;
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
