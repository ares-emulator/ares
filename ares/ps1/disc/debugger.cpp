auto Disc::Debugger::load(Node::Object parent) -> void {
  tracer.command = parent->append<Node::Debugger::Tracer::Notification>("Command", "CD");
  tracer.read = parent->append<Node::Debugger::Tracer::Notification>("Read", "CD");
}

auto Disc::Debugger::commandPrologue(u8 operation, maybe<u8> suboperation) -> void {
  if(!tracer.command->enabled()) return;
  if(operation == 0x19 && !suboperation) return;

  string name;

  if(operation == 0x01) {
    name = "GetStatus";
  }

  if(operation == 0x02) {
    name = "SetLocation";
  }

  if(operation == 0x03) {
    name = "Play";
  }

  if(operation == 0x04) {
    name = "FastForward";
  }

  if(operation == 0x05) {
    name = "Rewind";
  }

  if(operation == 0x06) {
    name = "ReadWithRetry";
  }

  if(operation == 0x07) {
    name = "MotorOn";
  }

  if(operation == 0x08) {
    name = "Stop";
  }

  if(operation == 0x09) {
    name = "Pause";
  }

  if(operation == 0x0a) {
    name = "Initialize";
  }

  if(operation == 0x0b) {
    name = "Mute";
  }

  if(operation == 0x0c) {
    name = "Unmute";
  }

  if(operation == 0x0d) {
    name = "SetFilter";
  }

  if(operation == 0x0e) {
    name = "SetMode";
  }

  if(operation == 0x0f) {
    name = "GetParameter";
  }

  if(operation == 0x10) {
    name = "GetLocationReading";
  }

  if(operation == 0x11) {
    name = "GetLocationPlaying";
  }

  if(operation == 0x12) {
    name = "SetSession";
  }

  if(operation == 0x13) {
    name = "GetFirstAndLastTrackNumbers";
  }

  if(operation == 0x14) {
    name = "GetTrackStart";
  }

  if(operation == 0x15) {
    name = "SeekData";
  }

  if(operation == 0x16) {
    name = "SeekCDDA";
  }

  if(operation == 0x19 && *suboperation == 0x20) {
    name = "TestControllerDate";
  }

  if(operation == 0x1a) {
    name = "GetID";
  }

  if(operation == 0x1b) {
    name = "ReadWithoutRetry";
  }

  if(!name) {
    if(!suboperation) name = {"$", hex(operation, 2L)};
    if( suboperation) name = {"Test$", hex(*suboperation, 2L)};
  }

  _command = name;
  _command.append("(");
  for(u32 index : range(disc.fifo.parameter.size())) {
    u8 data = disc.fifo.parameter.peek(index);
    _command.append("$", hex(data, 2L), ",");
  }
  _command.trimRight(",", 1L);
  _command.append(")");
}

auto Disc::Debugger::commandEpilogue(u8 operation, maybe<u8> suboperation) -> void {
  if(!tracer.command->enabled()) return;
  if(operation == 0x19 && !suboperation) return;

  _command.append(" = {");
  for(u32 index : range(disc.fifo.response.size())) {
    u8 data = disc.fifo.response.peek(index);
    _command.append("$", hex(data, 2L), ",");
  }
  _command.trimRight(",", 1L);
  _command.append("}");

  tracer.command->notify(_command);
}

auto Disc::Debugger::read(s32 lba) -> void {
  if(!tracer.read->enabled()) return;

  auto [minute, second, frame] = CD::MSF::fromLBA(lba);
  minute = CD::BCD::encode(minute);
  second = CD::BCD::encode(second);
  frame  = CD::BCD::encode(frame);
  tracer.read->notify({hex(minute, 2L), ":", hex(second, 2L), ":", hex(frame, 2L)});
}
