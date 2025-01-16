struct srmvs : Aleck64::GameConfig {
  u8 mahjongRow = 0;

  auto dipSwitches(Node::Object parent) -> void {
    Node::Setting::Boolean testMode = parent->append<Node::Setting::Boolean>("Test Mode", false, [&](auto value) {
      aleck64.dipSwitch[1].bit(7) = !value;
    });
    testMode->setDynamic(true);
    testMode->modify(testMode->value());
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
