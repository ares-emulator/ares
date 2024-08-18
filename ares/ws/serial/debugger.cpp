auto Serial::Debugger::load(Node::Object parent) -> void {
  properties.ports = parent->append<Node::Debugger::Properties>("Serial I/O");
  properties.ports->setQuery([&] { return ports(); });
}

auto Serial::Debugger::unload(Node::Object parent) -> void {
  parent->remove(properties.ports);
  properties.ports.reset();
}

auto Serial::Debugger::ports() -> string {
  string output;

  output.append("Serial: ");
  if(self.state.enable) {
    output.append("enabled, ", self.state.baudRate ? "38400 Hz" : "9600 Hz");
  } else {
    output.append("disabled");
  }
  output.append("\n");

  output.append("Serial TX in progress: ", self.state.txFull ? "yes" : "no", "\n");
  output.append("Serial RX available: ", self.state.rxFull ? "yes" : "no", "\n");
  output.append("Serial RX overrun: ", self.state.rxOverrun ? "yes" : "no", "\n");

  return output;
}
