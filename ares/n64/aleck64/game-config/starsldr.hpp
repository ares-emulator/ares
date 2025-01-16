struct starsldr : Aleck64::GameConfig {
  auto dipSwitches(Node::Object parent) -> void {
    Node::Setting::Boolean testMode = parent->append<Node::Setting::Boolean>("Test Mode", false, [&](auto value) {
      aleck64.dipSwitch[1].bit(7) = !value;
    });
    testMode->setDynamic(true);
    testMode->modify(testMode->value());

    Node::Setting::String joystickType = parent->append<Node::Setting::String>("Joystick Type", "2D", [&](auto value) {
      aleck64.dipSwitch[0].bit(7) = value == "3D";
    });
    joystickType->setAllowedValues({ "2D", "3D" });
    joystickType->setDynamic(true);
    joystickType->modify(joystickType->value());

    Node::Setting::String player = parent->append<Node::Setting::String>("Player", "3", [&](auto value) {
      if(value == "3") aleck64.dipSwitch[0].bit(3,4) = 3;
      if(value == "4") aleck64.dipSwitch[0].bit(3,4) = 2;
      if(value == "2") aleck64.dipSwitch[0].bit(3,4) = 1;
      if(value == "1") aleck64.dipSwitch[0].bit(3,4) = 0;
    });
    player->setAllowedValues({"4", "3", "2", "1"});
    player->setDynamic(true);
    player->modify(player->value());

    Node::Setting::String autoLevel = parent->append<Node::Setting::String>("Auto Level", "Normal", [&](auto value) {
      if(value == "Normal") aleck64.dipSwitch[0].bit(5,6) = 3;
      if(value == "Slow"  ) aleck64.dipSwitch[0].bit(5,6) = 2;
      if(value == "Fast1" ) aleck64.dipSwitch[0].bit(5,6) = 1;
      if(value == "Fast2" ) aleck64.dipSwitch[0].bit(5,6) = 0;
    });
    autoLevel->setAllowedValues({ "Normal", "Slow", "Fast1", "Fast2" });
    autoLevel->setDynamic(true);
    autoLevel->modify(autoLevel->value());

    Node::Setting::String coinage = parent->append<Node::Setting::String>("Coinage", "1 Coin 1 Credit", [&](auto value) {
      if(value == "1 Coin 1 Credit"  ) aleck64.dipSwitch[0].bit(0,2) = 7;
      if(value == "1 Coin 2 Credits" ) aleck64.dipSwitch[0].bit(0,2) = 6;
      if(value == "1 Coin 3 Credits" ) aleck64.dipSwitch[0].bit(0,2) = 5;
      if(value == "1 Coin 4 Credits" ) aleck64.dipSwitch[0].bit(0,2) = 4;
      if(value == "2 Coins 1 Credit" ) aleck64.dipSwitch[0].bit(0,2) = 3;
      if(value == "3 Coins 1 Credit") aleck64.dipSwitch[0].bit(0,2) = 2;
      if(value == "4 Coins 1 Credit") aleck64.dipSwitch[0].bit(0,2) = 1;
      if(value == "5 Coins 1 Credit") aleck64.dipSwitch[0].bit(0,2) = 0;
    });

    coinage->setAllowedValues({ "1 Coin 1 Credit",  "1 Coin 2 Credits",  "1 Coin 3 Credits",  "1 Coin 4 Credits",
                                "2 Coins 1 Credit", "3 Coins 1 Credit", "4 Coins 1 Credit", "5 Coins 1 Credit" });
    coinage->setDynamic(true);
    coinage->modify(coinage->value());

    Node::Setting::String language = parent->append<Node::Setting::String>("Language", "English", [&](auto value) {
      aleck64.dipSwitch[1].bit(6) = value == "English";
    });
    language->setAllowedValues({ "English", "Japanese" });
    language->setDynamic(true);
    language->modify(language->value());

    Node::Setting::Boolean demoSound = parent->append<Node::Setting::Boolean>("Demo Sound", true, [&](auto value) {
      aleck64.dipSwitch[1].bit(5) = !value;
    });
    demoSound->setDynamic(true);
    demoSound->modify(demoSound->value());

    Node::Setting::Boolean rapid = parent->append<Node::Setting::Boolean>("Rapid", false, [&](auto value) {
      aleck64.dipSwitch[1].bit(4) = value;
    });
    rapid->setDynamic(true);
    rapid->modify(rapid->value());

    Node::Setting::String extend = parent->append<Node::Setting::String>("Extend", "30000000", [&](auto value) {
      if(value == "30,000,000") aleck64.dipSwitch[1].bit(2,3) = 3;
      if(value == "50,000,000") aleck64.dipSwitch[1].bit(2,3) = 2;
      if(value == "70,000,000") aleck64.dipSwitch[1].bit(2,3) = 1;
      if(value == "None"      ) aleck64.dipSwitch[1].bit(2,3) = 0;
    });
    extend->setAllowedValues({ "30,000,000", "50,000,000", "70,000,000", "None" });
    extend->setDynamic(true);
    extend->modify(extend->value());

    Node::Setting::String difficulty = parent->append<Node::Setting::String>("Difficulty", "Normal", [&](auto value) {
      if(value == "Normal") aleck64.dipSwitch[1].bit(0,1) = 3;
      if(value == "Easy"  ) aleck64.dipSwitch[1].bit(0,1) = 2;
      if(value == "Hard1" ) aleck64.dipSwitch[1].bit(0,1) = 1;
      if(value == "Hard2" ) aleck64.dipSwitch[1].bit(0,1) = 0;
    });

    difficulty->setAllowedValues({ "Normal", "Easy", "Hard1", "Hard2" });
    difficulty->setDynamic(true);
    difficulty->modify(difficulty->value());
  }
};
