auto SI::Debugger::load(Node::Object parent) -> void {
  tracer.io = parent->append<Node::Notification>("I/O", "SI");
}

auto SI::Debugger::io(string_view message) -> void {
  if(tracer.io->enabled()) {
    tracer.io->notify(message);
  }
}
