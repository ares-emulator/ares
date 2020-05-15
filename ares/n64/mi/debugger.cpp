auto MI::Debugger::load(Node::Object parent) -> void {
  tracer.interrupt = parent->append<Node::Notification>("Interrupt", "RCP");
}

auto MI::Debugger::interrupt(string_view type) -> void {
  if(tracer.interrupt->enabled()) {
    tracer.interrupt->notify(type);
  }
}
