auto SI::Debugger::load(Node::Object parent) -> void {
  tracer.io = parent->append<Node::Debugger::Tracer::Notification>("I/O", "SI");
}

auto SI::Debugger::io(bool mode, u32 address, u32 data) -> void {
  static const std::vector<string> registerNames = {
    "SI_DRAM_ADDRESS",
    "SI_PIF_ADDRESS_READ64B",
    "SI_INT_ADDRESS_WRITE64B",
    "SI_RESERVED",
    "SI_PIF_ADDRESS_WRITE64B",
    "SI_INT_ADDRESS_READ64B",
    "SI_STATUS",
  };

  if(unlikely(tracer.io->enabled())) {
    string message;
    string name = (address < registerNames.size() ? registerNames[address] : string("SI_UNKNOWN"));
    if(mode == Read) {
      message = {nall::split(name, "|").front(), " => ", hex(data, 8L)};
    }
    if(mode == Write) {
      message = {nall::split(name, "|").back(), " <= ", hex(data, 8L)};
    }
    tracer.io->notify(message);
  }
}
