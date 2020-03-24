auto System::Controls::load(Node::Object parent, Node::Object from) -> void {
  node = Node::append<Node::Object>(parent, from, "Controls");
  from = Node::scan(parent = node, from);

  if(Model::WonderSwan() || Model::WonderSwanColor() || Model::SwanCrystal()) {
    y1     = Node::append<Node::Button>(parent, from, "Y1");
    y2     = Node::append<Node::Button>(parent, from, "Y2");
    y3     = Node::append<Node::Button>(parent, from, "Y3");
    y4     = Node::append<Node::Button>(parent, from, "Y4");
    x1     = Node::append<Node::Button>(parent, from, "X1");
    x2     = Node::append<Node::Button>(parent, from, "X2");
    x3     = Node::append<Node::Button>(parent, from, "X3");
    x4     = Node::append<Node::Button>(parent, from, "X4");
    b      = Node::append<Node::Button>(parent, from, "B");
    a      = Node::append<Node::Button>(parent, from, "A");
    start  = Node::append<Node::Button>(parent, from, "Start");
    volume = Node::append<Node::Button>(parent, from, "Volume");
  }

  if(Model::PocketChallengeV2()) {
    up     = Node::append<Node::Button>(parent, from, "Up");
    down   = Node::append<Node::Button>(parent, from, "Down");
    left   = Node::append<Node::Button>(parent, from, "Left");
    right  = Node::append<Node::Button>(parent, from, "Right");
    pass   = Node::append<Node::Button>(parent, from, "Pass");
    circle = Node::append<Node::Button>(parent, from, "Circle");
    clear  = Node::append<Node::Button>(parent, from, "Clear");
    view   = Node::append<Node::Button>(parent, from, "View");
    escape = Node::append<Node::Button>(parent, from, "Escape");
  }

  power = Node::append<Node::Button>(parent, from, "Power");
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
