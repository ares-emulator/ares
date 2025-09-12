auto AI::Debugger::load(Node::Object parent) -> void {
  tracer.io = parent->append<Node::Debugger::Tracer::Notification>("I/O", "AI");
}

auto AI::Debugger::io(bool mode, u32 address, u32 data) -> void {
  static const std::vector<string> registerNames = {
    "AI_DRAM_ADDRESS",
    "AI_LENGTH",
    "AI_CONTROL",
    "AI_STATUS",
    "AI_DACRATE",
    "AI_BITRATE",
  };

  if(unlikely(tracer.io->enabled())) {
    string message;
    string name = (address < registerNames.size() ? registerNames[address] : string("AI_UNKNOWN"));
    if(mode == Read) {
      message = {::nall::split(name, "|").front(), " => ", hex(data, 8L)};
    }
    if(mode == Write) {
      message = {::nall::split(name, "|").back(), " <= ", hex(data, 8L)};
    }
    tracer.io->notify(message);
  }
}
