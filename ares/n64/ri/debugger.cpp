auto RI::Debugger::load(Node::Object parent) -> void {
  tracer.io = parent->append<Node::Debugger::Tracer::Notification>("I/O", "RI");
}

auto RI::Debugger::io(bool mode, u32 address, u32 data) -> void {
  static const std::vector<string> registerNames = {
    "RI_MODE",
    "RI_CONFIG",
    "RI_CURRENT_LOAD",
    "RI_SELECT",
    "RI_REFRESH",
    "RI_LATENCY",
    "RI_RERROR",
    "RI_WERROR",
  };

  if(unlikely(tracer.io->enabled())) {
    string message;
    string name = (address < registerNames.size() ? registerNames[address] : string("RI_UNKNOWN"));
    if(mode == Read) {
      message = {nall::split(name, "|").front(), " => ", hex(data, 8L)};
    }
    if(mode == Write) {
      message = {nall::split(name, "|").back(), " <= ", hex(data, 8L)};
    }
    tracer.io->notify(message);
  }
}
