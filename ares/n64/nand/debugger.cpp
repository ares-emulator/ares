auto NAND::Debugger::load(Node::Object parent) -> void {
  string name = {"NAND", num};
  tracer.io = parent->append<Node::Debugger::Tracer::Notification>("I/O", name);
}

auto NAND::Debugger::command(Command cmd, n27 pageNumber, n10 length) -> void {
  
}
