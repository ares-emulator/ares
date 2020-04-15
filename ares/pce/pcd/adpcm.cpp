auto PCD::ADPCM::load(Node::Object parent) -> void {
  memory.allocate(64_KiB);
}

auto PCD::ADPCM::unload() -> void {
  memory.reset();
}
