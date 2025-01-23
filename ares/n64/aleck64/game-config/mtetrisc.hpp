struct mtetrisc : Aleck64::GameConfig {
  auto type() -> Aleck64::BoardType { return Aleck64::BoardType::E90; }

  auto dipSwitches(Node::Object parent) -> void {
    Node::Setting::Boolean testMode = parent->append<Node::Setting::Boolean>("Test Mode", false, [&](auto value) {
      aleck64.dipSwitch[1].bit(7) = !value;
    });
    testMode->setDynamic(true);
    testMode->modify(testMode->value());
  }
};
