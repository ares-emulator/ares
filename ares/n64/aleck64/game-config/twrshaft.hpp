struct twrshaft : Aleck64::GameConfig {
  auto ioPortControls(int port) -> n32 {
    n32 value = 0xffff'ffff;

    if(port == 1) {
      value.bit( 0) = !aleck64.controls.p1up->value();
      value.bit( 1) = !aleck64.controls.p1down->value();
      value.bit( 2) = !aleck64.controls.p1left->value();
      value.bit( 3) = !aleck64.controls.p1right->value();
      value.bit( 4) = !aleck64.controls.p1[0]->value();
    }

    if(port == 2) {
      value.bit(16) = !aleck64.controls.p1start->value();
      value.bit(18) = !aleck64.controls.p1coin->value();
      value.bit(19) = !aleck64.controls.p2coin->value();
      value.bit(20) = !aleck64.controls.service->value();
      value.bit(21) = !aleck64.controls.test->value();
    }

    return value;
  }

  auto dipSwitches(Node::Object parent) -> void {
    Node::Setting::Boolean testMode = parent->append<Node::Setting::Boolean>("Test Mode", false, [&](auto value) {
      aleck64.dipSwitch[1].bit(7) = !value;
    });
    testMode->setDynamic(true);
    testMode->modify(testMode->value());
  }
};
