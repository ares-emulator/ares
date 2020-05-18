auto MI::Debugger::load(Node::Object parent) -> void {
  tracer.interrupt = parent->append<Node::Notification>("Interrupt", "RCP");
  tracer.io = parent->append<Node::Notification>("I/O", "MI");
}

auto MI::Debugger::interrupt(string_view type) -> void {
  if(tracer.interrupt->enabled()) {
    tracer.interrupt->notify(type);
  }
}

auto MI::Debugger::io(string_view message) -> void {
  if(tracer.io->enabled()) {
    tracer.io->notify(message);
  }
}
