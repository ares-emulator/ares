struct doncdoon : Aleck64::GameConfig {
  auto dipSwitches(Node::Object parent) -> void {
    Node::Setting::Boolean testMode = parent->append<Node::Setting::Boolean>("Test Mode", false, [&](auto value) {
      aleck64.dipSwitch[0].bit(0) = !value;
    });
    testMode->setDynamic(true);
    testMode->modify(testMode->value());
  }
};
