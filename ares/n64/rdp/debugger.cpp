auto RDP::Debugger::load(Node::Object parent) -> void {
  tracer.command = parent->append<Node::Notification>("Command", "RDP");
  tracer.io = parent->append<Node::Notification>("I/O", "RDP");
}

auto RDP::Debugger::command(string_view message) -> void {
  if(tracer.command->enabled()) {
    tracer.command->notify(message);
  }
}

auto RDP::Debugger::io(string_view message) -> void {
  if(tracer.io->enabled()) {
    tracer.io->notify(message);
  }
}
