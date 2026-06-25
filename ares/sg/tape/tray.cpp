auto TapeDeckTray::load(Node::Object parent) -> void {
  port = parent->append<Node::Port>("Tray");
  port->setFamily("SC-3000");
  port->setType("Tape");
  port->setHotSwappable(true);
  port->setAllocate([&](auto name) { return tape.allocate(port); });
  port->setConnect([&] { return tape.connect(); });
  port->setDisconnect([&] { return tape.disconnect(); });
}

auto TapeDeckTray::unload() -> void {
  tape.disconnect();
  port = {};
}
