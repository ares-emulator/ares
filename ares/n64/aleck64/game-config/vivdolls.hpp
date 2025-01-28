struct vivdolls : Aleck64::GameConfig {
  auto dipSwitches(Node::Object parent) -> void {
    Node::Setting::Boolean testMode = parent->append<Node::Setting::Boolean>("Test Mode", false, [&](auto value) {
      aleck64.dipSwitch[1].bit(7) = !value;
    });
    testMode->setDynamic(true);
    testMode->modify(testMode->value());

    Node::Setting::String coinage = parent->append<Node::Setting::String>("Coinage", "1 Coin 1 Credit", [&](auto value) {
      if(value == "1 Coin 1 Credit" ) aleck64.dipSwitch[0].bit(0,2) = 7;
      if(value == "1 Coin 2 Credits") aleck64.dipSwitch[0].bit(0,2) = 3;
      if(value == "1 Coin 3 Credits") aleck64.dipSwitch[0].bit(0,2) = 5;
      if(value == "1 Coin 4 Credits") aleck64.dipSwitch[0].bit(0,2) = 1;
      if(value == "2 Coins 1 Credit") aleck64.dipSwitch[0].bit(0,2) = 6;
      if(value == "3 Coins 1 Credit") aleck64.dipSwitch[0].bit(0,2) = 2;
      if(value == "4 Coins 1 Credit") aleck64.dipSwitch[0].bit(0,2) = 4;
      if(value == "5 Coins 1 Credit") aleck64.dipSwitch[0].bit(0,2) = 0;
    });

    coinage->setAllowedValues({ "1 Coin 1 Credit",  "1 Coin 2 Credits",  "1 Coin 3 Credits",  "1 Coin 4 Credits",
                                "2 Coins 1 Credit", "3 Coins 1 Credit", "4 Coins 1 Credit", "5 Coins 1 Credit" });
    coinage->setDynamic(true);
    coinage->modify(coinage->value());

    Node::Setting::String qualifyArea = parent->append<Node::Setting::String>("Qualify Area", "70%", [&](auto value) {
      if(value == "70%") aleck64.dipSwitch[1].bit(5) = 0;
      if(value == "80%") aleck64.dipSwitch[1].bit(5) = 1;
    });
    qualifyArea->setAllowedValues({ "70%", "80%" });
    qualifyArea->setDynamic(true);
    qualifyArea->modify(qualifyArea->value());

    Node::Setting::String controls = parent->append<Node::Setting::String>("Controls", "JOYSTICK", [&](auto value) {
      if(value == "JAMMASTICK") aleck64.dipSwitch[1].bit(5) = 0;
      if(value == "JOYSTICK"  ) aleck64.dipSwitch[1].bit(5) = 1;
    });
    controls->setAllowedValues({ "JAMMASTICK", "JOYSTICK" });
    controls->setDynamic(true);
    controls->modify(controls->value());

    Node::Setting::String lives = parent->append<Node::Setting::String>("Lives", "4", [&](auto value) {
      if(value == "4") aleck64.dipSwitch[1].bit(2,3) = 3;
      if(value == "3") aleck64.dipSwitch[1].bit(2,3) = 2;
      if(value == "2") aleck64.dipSwitch[1].bit(2,3) = 1;
      if(value == "1") aleck64.dipSwitch[1].bit(2,3) = 0;
    });
    lives->setAllowedValues({ "4", "3", "2", "1" });
    lives->setDynamic(true);
    lives->modify(lives->value());

    Node::Setting::String difficulty = parent->append<Node::Setting::String>("Difficulty", "Normal", [&](auto value) {
      if(value == "Normal" ) aleck64.dipSwitch[1].bit(0,1) = 3;
      if(value == "Easy"   ) aleck64.dipSwitch[1].bit(0,1) = 2;
      if(value == "Hard"   ) aleck64.dipSwitch[1].bit(0,1) = 1;
      if(value == "Hardest") aleck64.dipSwitch[1].bit(0,1) = 0;
    });
    difficulty->setAllowedValues({ "Normal", "Easy", "Hard", "Hardest" });
    difficulty->setDynamic(true);
    difficulty->modify(difficulty->value());
  }
};
