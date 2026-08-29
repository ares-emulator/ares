QuadTari::QuadTari(Node::Port parent) : first(*this), second(*this) {
  node = parent->append<Node::Peripheral>("QuadTari");

  auto left = parent->name() == "Controller Port 1";
  first.load(node, left ? "QuadTari Player 1" : "QuadTari Player 2");
  second.load(node, left ? "QuadTari Player 3" : "QuadTari Player 4");
}

QuadTari::~QuadTari() {
  first.unload();
  second.unload();
}

auto QuadTari::Slot::load(Node::Peripheral parent, string name) -> void {
  port = parent->append<Node::Port>(name);
  port->setFamily(system.name());
  port->setType("Controller");
  port->setHotSwappable(true);
  port->setAllocate([&](auto name) { return allocate(name); });
  port->setDisconnect([&] {
    save();
    device.reset();
  });
  port->setSupported({"Gamepad", "Paddles", "Driving", "SaveKey", "AtariVox"});
  port->allocate("Gamepad");
  port->connect();
}

auto QuadTari::Slot::unload() -> void {
  save();
  device = {};
  port = {};
}

auto QuadTari::Slot::allocate(string name) -> Node::Peripheral {
  save();
  device = {};
  if(name != "Gamepad" && name != "Paddles" && name != "Driving" && name != "SaveKey" && name != "AtariVox") {
    return {};
  }
  device = ControllerPort::create(port, name, ControllerPort::Role::QuadTariChild);
  if(device) {
    device->write(owner.output);
    device->controlWrite(owner.control);
    return device->node;
  }
  return {};
}

auto QuadTari::Slot::save() -> void {
  if(device) device->save();
}

auto QuadTari::Slot::power(bool reset) -> void {
  if(device) device->power(reset);
}

auto QuadTari::Slot::serialize(serializer& s) -> void {
  if(device) device->serialize(s);
}

auto QuadTari::save() -> void {
  first.save();
  second.save();
}

auto QuadTari::power(bool reset) -> void {
  first.power(reset);
  second.power(reset);
  dumped = 0;
  selected = 0;
  pending = 0;
  settling = 0;
  output = 0x0f;
  control = 0x0f;
}

auto QuadTari::poll() -> void {
  if(first.device) first.device->poll();
  if(second.device) second.device->poll();
}

auto QuadTari::frame() -> void {
  if(first.device) first.device->frame();
  if(second.device) second.device->frame();
}

auto QuadTari::clock() -> void {
  if(settling && !--settling) {
    selected = pending;
    synchronizeActive();
  }
}

auto QuadTari::vblank(n1 dumped) -> void {
  if(this->dumped == dumped) return;
  this->dumped = dumped;
  pending = dumped;
  settling = SettlingClocks;
}

auto QuadTari::read() -> n8 {
  if(auto device = active()) return device->read();
  return 0xff;
}

auto QuadTari::write(n8 data) -> void {
  output = data;
  if(auto device = active()) device->write(data);
}

auto QuadTari::controlWrite(n8 data) -> void {
  control = data;
  if(auto device = active()) device->controlWrite(data);
}

auto QuadTari::readAnalog(n1 index) -> AnalogConnection {
  return index ? AnalogConnection::vcc() : AnalogConnection::ground();
}

auto QuadTari::serialize(serializer& s) -> void {
  s(dumped);
  s(selected);
  s(pending);
  s(settling);
  s(output);
  s(control);
  first.serialize(s);
  second.serialize(s);

  if(s.reading() && settling > SettlingClocks) {
    dumped = 0;
    selected = 0;
    pending = 0;
    settling = 0;
    output = 0x0f;
    control = 0x0f;
    synchronizeActive();
  }
}

auto QuadTari::active() -> Controller* {
  auto& slot = selected ? second : first;
  return slot.device.get();
}

auto QuadTari::synchronizeActive() -> void {
  if(auto device = active()) {
    device->write(output);
    device->controlWrite(control);
  }
}
