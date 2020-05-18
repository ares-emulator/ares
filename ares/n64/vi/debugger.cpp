auto VI::Debugger::load(Node::Object parent) -> void {
  tracer.io = parent->append<Node::Notification>("I/O", "VI");
}

auto VI::Debugger::io(string_view message) -> void {
  if(tracer.io->enabled()) {
    tracer.io->notify(message);
  }
}
