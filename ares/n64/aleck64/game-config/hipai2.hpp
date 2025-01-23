struct hipai2 : Aleck64::GameConfig {
  u8 mahjongRow = 0;

  auto dipSwitches(Node::Object parent) -> void {
    Node::Setting::Boolean testMode = parent->append<Node::Setting::Boolean>("Test Mode", false, [&](auto value) {
      aleck64.dipSwitch[1].bit(7) = !value;
    });
    testMode->setDynamic(true);
    testMode->modify(testMode->value());

    Node::Setting::Boolean freePlay = parent->append<Node::Setting::Boolean>("Free Play", false, [&](auto value) {
      aleck64.dipSwitch[0].bit(7) = !value;
    });
    freePlay->setDynamic(true);
    freePlay->modify(freePlay->value());

    Node::Setting::Boolean backupSettings = parent->append<Node::Setting::Boolean>("Backup Settings", false, [&](auto value) {
      aleck64.dipSwitch[0].bit(6) = !value;
    });
    backupSettings->setDynamic(true);
    backupSettings->modify(backupSettings->value());

    Node::Setting::String coinage = parent->append<Node::Setting::String>("Coinage", "1 Coin 1 Credit", [&](auto value) {
      if(value == "1 Coin 1 Credit"  ) aleck64.dipSwitch[0].bit(0,2) = 7;
      if(value == "1 Coin 2 Credits" ) aleck64.dipSwitch[0].bit(0,2) = 3;
      if(value == "1 Coin 3 Credits" ) aleck64.dipSwitch[0].bit(0,2) = 5;
      if(value == "1 Coin 4 Credits" ) aleck64.dipSwitch[0].bit(0,2) = 4;
      if(value == "1 Coins 2 Credits") aleck64.dipSwitch[0].bit(0,2) = 6;
      if(value == "3 Coins 1 Credit" ) aleck64.dipSwitch[0].bit(0,2) = 2;
      if(value == "4 Coins 1 Credit" ) aleck64.dipSwitch[0].bit(0,2) = 1;
      if(value == "5 Coins 1 Credit" ) aleck64.dipSwitch[0].bit(0,2) = 0;
    });
    coinage->setAllowedValues({"1 Coin 1 Credit" , "1 Coin 2 Credits" , "1 Coin 3 Credits" , "1 Coin 4 Credits",
                               "1 Coins 2 Credit", "3 Coins 1 Credit", "4 Coins 1 Credit", "5 Coins 1 Credit",
    });
    coinage->setDynamic(true);
    coinage->modify(coinage->value());

    Node::Setting::Boolean demoSounds = parent->append<Node::Setting::Boolean>("Demo Sounds", true, [&](auto value) {
      aleck64.dipSwitch[1].bit(5) = !value;
    });
    demoSounds->setDynamic(true);
    demoSounds->modify(demoSounds->value());

    Node::Setting::Boolean allowContinue = parent->append<Node::Setting::Boolean>("Allow Continue", false, [&](auto value) {
      aleck64.dipSwitch[1].bit(4) = value;
    });
    allowContinue->setDynamic(true);
    allowContinue->modify(allowContinue->value());

    Node::Setting::Boolean kuitan = parent->append<Node::Setting::Boolean>("Kuitan", false, [&](auto value) {
      aleck64.dipSwitch[1].bit(3) = value;
    });
    kuitan->setDynamic(true);
    kuitan->modify(kuitan->value());

    Node::Setting::String difficulty = parent->append<Node::Setting::String>("Difficulty", "Normal", [&](auto value) {
      if(value == "Normal"   ) aleck64.dipSwitch[1].bit(0,2) = 7;
      if(value == "Easiest"  ) aleck64.dipSwitch[1].bit(0,2) = 6;
      if(value == "Very Easy") aleck64.dipSwitch[1].bit(0,2) = 5;
      if(value == "Easy"     ) aleck64.dipSwitch[1].bit(0,2) = 4;
      if(value == "Normal+"  ) aleck64.dipSwitch[1].bit(0,2) = 3;
      if(value == "Hard"     ) aleck64.dipSwitch[1].bit(0,2) = 2;
      if(value == "Very Hard") aleck64.dipSwitch[1].bit(0,2) = 1;
      if(value == "Most Hard") aleck64.dipSwitch[1].bit(0,2) = 0;
    });
    difficulty->setAllowedValues({"Normal", "Easiest", "Very Easy", "Easy", "Normal+", "Hard", "Very Hard", "Most Hard"});
    difficulty->setDynamic(true);
    difficulty->modify(difficulty->value());
  }

  auto readExpansionPort() -> u32 {
    n32 value = 0xffff'ffff;
    value.bit(16, 23) = aleck64.controls.mahjong(mahjongRow);
    return value;
  }

  auto writeExpansionPort(n32 data) -> void {
    mahjongRow = data.bit(8, 15);
  };
};
