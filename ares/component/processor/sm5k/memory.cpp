auto SM5K::fetch() -> n8 {
  tick();
  return ROM[PC++];
}
