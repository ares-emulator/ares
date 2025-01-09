struct mayjin3 : Aleck64::GameConfig {
  auto controllerButton(int playerIndex, string button) -> bool {
    if(playerIndex == 1) {
      if(button == "Up"     ) return aleck64.controls.p1up->value();
      if(button == "Down"   ) return aleck64.controls.p1down->value();
      if(button == "Left"   ) return aleck64.controls.p1left->value();
      if(button == "Right"  ) return aleck64.controls.p1right->value();
      if(button == "Start"  ) return aleck64.controls.p1start->value();
      if(button == "A"      ) return aleck64.controls.p1[0]->value();
      if(button == "B"      ) return aleck64.controls.p1[1]->value();
      if(button == "R"      ) return aleck64.controls.p1[2]->value();
      if(button == "C-Right") return aleck64.controls.p1[3]->value();
    }

    if(playerIndex == 2) {
      if(button == "Up"     ) return aleck64.controls.p2up->value();
      if(button == "Down"   ) return aleck64.controls.p2down->value();
      if(button == "Left"   ) return aleck64.controls.p2left->value();
      if(button == "Right"  ) return aleck64.controls.p2right->value();
      if(button == "Start"  ) return aleck64.controls.p2start->value();
      if(button == "A"      ) return aleck64.controls.p2[0]->value();
      if(button == "B"      ) return aleck64.controls.p2[1]->value();
      if(button == "R"      ) return aleck64.controls.p2[2]->value();
      if(button == "C-Right") return aleck64.controls.p2[3]->value();
    }

    return {};
  }

  auto controllerAxis(int playerIndex, string axis) -> s64 {
    return {};
  }

  auto ioPortControls(int port) -> n32 {
    n32 value = 0xffff'ffff;

    if(port == 2) {
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
