auto Bus::load(Node::Object parent) -> void {
  //4_MiB internal
  //4_MiB expansion pak
  rdram.allocate(4_MiB + 4_MiB);
}

auto Bus::unload() -> void {
  rdram.reset();
}
