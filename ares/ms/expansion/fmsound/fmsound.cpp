FMSoundUnit::FMSoundUnit(Node::Port parent) {
  node = parent->append<Node::Peripheral>("FM Sound Unit");
  opll.load(node);
  opll.power();
}

FMSoundUnit::~FMSoundUnit() {
  opll.unload();
}

auto FMSoundUnit::serialize(serializer& s) -> void {
  s(opll);
}
