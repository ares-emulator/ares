struct _11beat : Aleck64::GameConfig {
  _11beat() {
    //Define port bit indexes for special inputs
    p1coin = 18;
    p2coin = 19;
    service = 21;
  }

  auto controllerButton(int playerIndex, string button) -> bool {
    //NOTE: D-Pad must be disconnected (high) for this game to boot
    if(playerIndex == 1) {
      if(button == "Up"     ) return 1;
      if(button == "Down"   ) return 1;
      if(button == "Left"   ) return 1;
      if(button == "Right"  ) return 1;
      if(button == "Start"  ) return aleck64.controls.p1start->value();
      if(button == "A"      ) return aleck64.controls.p1[0]->value();
      if(button == "B"      ) return aleck64.controls.p1[1]->value();
      if(button == "R"      ) return aleck64.controls.p1[2]->value();
      if(button == "C-Right") return aleck64.controls.p1[3]->value();
    }

    if(playerIndex == 2) {
      if(button == "Up"     ) return 1;
      if(button == "Down"   ) return 1;
      if(button == "Left"   ) return 1;
      if(button == "Right"  ) return 1;
      if(button == "Start"  ) return aleck64.controls.p2start->value();
      if(button == "A"      ) return aleck64.controls.p2[0]->value();
      if(button == "B"      ) return aleck64.controls.p2[1]->value();
      if(button == "R"      ) return aleck64.controls.p2[2]->value();
      if(button == "C-Right") return aleck64.controls.p2[3]->value();
    }

    return {};
  }

  auto controllerAxis(int playerIndex, string axis) -> s64 {
    if(playerIndex == 1) {
      if(axis == "X") return aleck64.controls.p1x->value();
      if(axis == "Y") return aleck64.controls.p1y->value();
    }

    if(playerIndex == 2) {
      if(axis == "X") return aleck64.controls.p2x->value();
      if(axis == "Y") return aleck64.controls.p2y->value();
    }

    return {};
  }

  auto dipSwitches(Node::Object parent) -> void {
    Node::Setting::Boolean testMode = parent->append<Node::Setting::Boolean>("Test Mode", false, [&](auto value) {
      aleck64.dipSwitch[1].bit(7) = !value;
    });
    testMode->setDynamic(true);
    testMode->modify(testMode->value());
  }
};