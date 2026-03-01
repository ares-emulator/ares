auto Disc::Debugger::load(Node::Object parent) -> void {
  tracer.command = parent->append<Node::Debugger::Tracer::Notification>("Command", "CD");
  tracer.read = parent->append<Node::Debugger::Tracer::Notification>("Read", "CD");
}

auto Disc::Debugger::commandPrologue(u8 operation, maybe<u8> suboperation) -> void {
  if(!tracer.command->enabled()) return;
  if(operation == 0x19 && !suboperation) return;

  string name;

  if(operation == 0x01) {
    name = "Nop";
  }

  if(operation == 0x02) {
    name = "Setloc";
  }

  if(operation == 0x03) {
    name = "Play";
  }

  if(operation == 0x04) {
    name = "Forward";
  }

  if(operation == 0x05) {
    name = "Backward";
  }

  if(operation == 0x06) {
    name = "ReadN";
  }

  if(operation == 0x07) {
    name = "Standby";
  }

  if(operation == 0x08) {
    name = "Stop";
  }

  if(operation == 0x09) {
    name = "Pause";
  }

  if(operation == 0x0a) {
    name = "Init";
  }

  if(operation == 0x0b) {
    name = "Mute";
  }

  if(operation == 0x0c) {
    name = "Demute";
  }

  if(operation == 0x0d) {
    name = "SetFilter";
  }

  if(operation == 0x0e) {
    name = "SetMode";
  }

  if(operation == 0x0f) {
    name = "Getparam";
  }

  if(operation == 0x10) {
    name = "GetlocL";
  }

  if(operation == 0x11) {
    name = "GetlocP";
  }

  if(operation == 0x12) {
    name = "SetSession";
  }

  if(operation == 0x13) {
    name = "GetTN";
  }

  if(operation == 0x14) {
    name = "GetTD";
  }

  if(operation == 0x15) {
    name = "SeekL";
  }

  if(operation == 0x16) {
    name = "SeekP";
  }

  if(operation == 0x19 && *suboperation == 0x20) {
    name = "TestControllerDate";
  }

  if(operation == 0x1a) {
    name = "GetID";
  }

  if(operation == 0x1b) {
    name = "ReadS";
  }

  if(operation == 0x1c) {
    name = "Reset";
  }

  if(operation == 0x1e) {
    name = "ReadToc";
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

  auto msf = CD::MSF::fromLBA(lba);
  u8 minute = BCD::encode(msf.minute);
  u8 second = BCD::encode(msf.second);
  u8 frame  = BCD::encode(msf.frame);
  tracer.read->notify({hex(minute, 2L), ":", hex(second, 2L), ":", hex(frame, 2L)});
}
