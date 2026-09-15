auto Disc::status() -> u8 {
  n8 data;
  data.bit(0) = ssr.error;
  data.bit(1) = ssr.motorOn;
  data.bit(2) = ssr.seekError;
  data.bit(3) = ssr.idError;
  data.bit(4) = ssr.shellOpen;
  data.bit(5) = ssr.reading;
  data.bit(6) = (drive.seeking != 0 && drive.seekVisible) || drive.sessionChange.pending;
  data.bit(7) = ssr.playingCDDA;
  if(drive.streamStarting) data = (data & 0x1f) | drive.startingStatus;
  if(drive.activeStatusCleared) data &= 0x1f;
  return data;
}

auto Disc::mode() -> u8 {
  n8 mode;
  mode.bit(0) = drive.mode.cdda;
  mode.bit(1) = drive.mode.autoPause;
  mode.bit(2) = drive.mode.report;
  mode.bit(3) = drive.mode.xaFilter;
  mode.bit(4) = drive.mode.ignore;
  mode.bit(5) = drive.mode.sectorSize;
  mode.bit(6) = drive.mode.xaADPCM;
  mode.bit(7) = drive.mode.speed;
  return mode;
}

auto Disc::error(u8 code) -> void {
  queueResponse(ResponseType::Error, {u8(status() | 0x01), code});
}

auto Disc::ack() -> void {
  ssr.error = 0;
  ssr.seekError = 0;
  queueResponse(ResponseType::Acknowledge, {status()});
}

auto Disc::queueResponse(ResponseType type, std::initializer_list<u8> response, bool asynchronous) -> void {
  asynchronous |= type == ResponseType::Ready || type == ResponseType::Complete || type == ResponseType::End;
  if(!asynchronous) {
    fifo.response.flush();
    for(auto value : response) fifo.response.write(value);
    publishResponse(type);
    return;
  }

  // Repeated sector arrivals do not queue another copy of an already asserted INT1.
  if(type == ResponseType::Ready) fifo.deferred = {};
  if(irq.flag == (u8)type) return;
  fifo.deferred.type = type;
  fifo.deferred.data.flush();
  for(auto value : response) fifo.deferred.data.write(value);
  fifo.deferred.scheduled = false;
  scheduleAsyncResponse();
}

auto Disc::publishResponse(ResponseType type) -> void {
  if(type == ResponseType::Ready && fifo.sectorPending) {
    fifo.sectorRead = fifo.sectorWrite;
    fifo.sectorPending = false;
    fifo.sectorNeedsFilter = false;
    fifo.hostLoaded = false;
    if(!io.sectorBufferReadRequest || fifo.data.empty()) fifo.data.flush();
  }
  for(u32 index : range(16)) fifo.responseLatch[index] = index < fifo.response.size() ? fifo.response.peek(index) : 0;
  fifo.responsePosition = 0;
  irq.flag = (irq.flag & 0x18) | (u8)type;
  irq.poll();
  updateCommandEvent();
}

auto Disc::scheduleAsyncResponse(u32 delay, bool force) -> void {
  if(fifo.deferred.type == ResponseType::None || (irq.pending() && !delay) || fifo.deferred.scheduled) return;
  if(!force && !delay && command.first.pending) return;
  fifo.deferred.counter = delay ? delay : irq.acknowledgeAge < 1000 ? 500 : 0;
  fifo.deferred.scheduled = true;
  updateCommandEvent();
  flushDeferredResponse();
}

auto Disc::flushDeferredResponse() -> void {
  if(!fifo.deferred.scheduled || fifo.deferred.counter > 0 || irq.pending()) return;
  auto type = fifo.deferred.type;
  if(type == ResponseType::Ready && fifo.sectorPending && fifo.sectorNeedsFilter && drive.mode.xaFilter
    && (fifo.sectorFile != cdxa.filter.file || fifo.sectorChannel != cdxa.filter.channel)) {
    // H2: only the deferred second data-delivery attempt applies file/channel filtering.
    fifo.sectorPending = false;
    fifo.sectorNeedsFilter = false;
    fifo.deferred = {};
    updateCommandEvent();
    return;
  }
  fifo.response.flush();
  while(!fifo.deferred.data.empty()) fifo.response.write(fifo.deferred.data.read(0));
  fifo.deferred = {};
  publishResponse(type);
}

auto Disc::updateCommandEvent() -> void {
  command.active = command.first.pending && !irq.pending() && fifo.deferred.type == ResponseType::None;
}

auto Disc::beginCommand(u8 operation) -> void {
  u32 delay = operation == 0x0a ? 80'000 : canReadMedia() ? 25'000 : 15'000;
  if(command.first.pending) {
    // Reference heuristic, not an HC05 microcode model.
    if(operation < 0x20 && command.first.command < 0x20
      && parameterCount(command.first.command).minimum > parameterCount(operation).minimum) {
      fifo.parameter.flush();
      return;
    }
    if(command.active) {
      delay = delay > command.elapsed ? delay - command.elapsed : 1;
      scheduleAsyncResponse(0, true);
    }
  }
  command.first = {operation, 1, (s32)delay};
  command.elapsed = 0;
  updateCommandEvent();
  ssr.error = 0;
}

auto Disc::scheduleSecondResponse(u8 operation, u32 clocks) -> void {
  command.second = {operation, 1, (s32)clocks};
}

auto Disc::parameterCount(u8 operation) -> ParameterCount {
  switch(operation) {
  case 0x02: return {3, 3};  //Setloc
  case 0x03: return {0, 1};  //Play
  case 0x0d: case 0x1d: return {2, 2};  //Setfilter, GetQ
  case 0x0e: case 0x12: case 0x14: return {1, 1};  //Setmode, SetSession, GetTD
  case 0x19: return {1, 16};  //Test subcommands consume their own parameters
  case 0x1c: return {0, 16};  //H2: Reset executes regardless of parameter count.
  case 0x1f: return {6, 16};  //VideoCD, unsupported but with defined parameter validation
  default: return {0, 0};
  }
}

auto Disc::executeCommand(u8 operation, bool secondResponse) -> void {
  debugger.commandPrologue(operation);

  // Validate once, before any first-response side effects. Later invocations have no parameter FIFO.
  if(!secondResponse) {
    auto count = parameterCount(operation);
    if(operation >= 0x20 || fifo.parameter.size() < count.minimum || fifo.parameter.size() > count.maximum) {
      error(operation >= 0x20 ? ErrorCode_InvalidCommand : ErrorCode_InvalidParameterCount);
      fifo.parameter.flush();
      debugger.commandEpilogue(operation);
      return;
    }
  }

  switch(operation) {
  case 0x00: commandInvalid(); break;  //Sync
  case 0x01: commandNop(); break;
  case 0x02: commandSetLoc(); break;
  case 0x03: commandPlay(); break;
  case 0x04: commandForward(); break;
  case 0x05: commandBackward(); break;
  case 0x06: commandReadN(); break;
  case 0x07: commandMotorOn(secondResponse); break;
  case 0x08: commandStop(secondResponse); break;
  case 0x09: commandPause(secondResponse); break;
  case 0x0a: commandInit(secondResponse); break;
  case 0x0b: commandMute(); break;
  case 0x0c: commandDemute(); break;
  case 0x0d: commandSetFilter(); break;
  case 0x0e: commandSetMode(); break;
  case 0x0f: commandGetParam(); break;
  case 0x10: commandGetlocL(); break;
  case 0x11: commandGetlocP(); break;
  case 0x12: commandSetSession(secondResponse); break;
  case 0x13: commandGetTN(); break;
  case 0x14: commandGetTD(); break;
  case 0x15: commandSeekL(); break;
  case 0x16: commandSeekP(); break;
  case 0x17: commandInvalid(); break;  //SetClock
  case 0x18: commandInvalid(); break;  //GetClock
  case 0x19: commandTest(); break;
  case 0x1a: commandGetID(secondResponse); break;
  case 0x1b: commandReadS(); break;
  case 0x1c: softReset(); ack(); break;
  case 0x1d: commandInvalid(); break;
  case 0x1e: commandReadToc(secondResponse); break;
  case 0x1f: commandInvalid(); break;
  default: commandInvalid(); break;
  }

  if(!secondResponse && operation != 0x1f) fifo.parameter.flush();
  debugger.commandEpilogue(operation);
}

//0x19
auto Disc::commandTest() -> void {
  u8 operation = 0x19;
  u8 suboperation = fifo.parameter.read(0);
  debugger.commandPrologue(operation, suboperation);

  switch(suboperation) {
  case 0x04: commandTestStartReadSCEX(); break;
  case 0x05: commandTestStopReadSCEX(); break;
  case 0x20: commandTestControllerDate(); break;
  case 0x22:
    if(controllerVersion < 2) { error(ErrorCode_InvalidParameterValue); break; }
    if(controllerVersion == 4) {
      queueResponse(ResponseType::Acknowledge, {'f', 'o', 'r', ' ', 'U', 'S', '/', 'A', 'E', 'P'});
      break;
    }
    if(controllerVersion == 6) {
      queueResponse(ResponseType::Acknowledge, {'f', 'o', 'r', ' ', 'N', 'E', 'T', 'N', 'A'});
      break;
    }
    if(Region::NTSCJ()) queueResponse(ResponseType::Acknowledge, {'f', 'o', 'r', ' ', 'J', 'a', 'p', 'a', 'n'});
    if(Region::NTSCU()) queueResponse(ResponseType::Acknowledge, {'f', 'o', 'r', ' ', 'U', '/', 'C'});
    if(Region::PAL()) queueResponse(ResponseType::Acknowledge, {'f', 'o', 'r', ' ', 'E', 'u', 'r', 'o', 'p', 'e'});
    break;
  case 0x60:
    if(fifo.parameter.size() < 2) error(ErrorCode_InvalidParameterCount);
    else queueResponse(ResponseType::Acknowledge, {0});  // Reference placeholder; no controller RAM is emulated.
    break;
  default: commandInvalid(); break;
  }

  debugger.commandEpilogue(operation, suboperation);
}

//0x00
auto Disc::commandInvalid() -> void {
  error(ErrorCode_InvalidCommand);
}

//0x01
auto Disc::commandNop() -> void {
  if(!fifo.parameter.empty()) {
    error(ErrorCode_InvalidParameterCount);
    return;
  }

  ack();

  if(manualLidControl ? !lidOpen : canReadMedia()) ssr.shellOpen = 0;
}

//0x02
auto Disc::commandSetLoc() -> void {
  if(fifo.parameter.size() != 3) {
    error(ErrorCode_InvalidParameterCount);
    return;
  }

  for(auto n : range(3)) {
    if(!BCD::valid(fifo.parameter.peek(n))) {
      error(ErrorCode_InvalidParameterValue);
      return;
    }
  }

  u8 minute = BCD::decode(fifo.parameter.read(0));
  u8 second = BCD::decode(fifo.parameter.read(0));
  u8 frame  = BCD::decode(fifo.parameter.read(0));

  if(minute > 99 || second > 59 || frame > 74) {
    error(ErrorCode_InvalidParameterValue);
    return;
  }

  ack();

  drive.lba.request = CD::MSF(minute, second, frame).toLBA();
  drive.lba.pending = 1;
}

//0x03
auto Disc::commandPlay() -> void {
  if(!canReadMedia()) return error(ErrorCode_CannotRespondYet);
  u8 trackID = fifo.parameter.read(0);
  if(trackID) {
    auto track = session.track(BCD::decode(trackID));
    if(!track || !track->index(1)) return error(ErrorCode_InvalidParameterValue);
    drive.lba.request = track->index(1)->lba;
    drive.lba.pending = 1;
  }

  ack();
  bool continuing = ssr.playingCDDA || drive.pendingOperation == Drive::PendingOperation::Play;
  s32 next = drive.seeking ? drive.lba.seek : drive.lba.current;
  if(!trackID && continuing && (!drive.lba.pending || drive.lba.request == next)) {
    drive.lba.pending = 0;
    cdda.scanStep = 0;
    cdda.playMode = CDDA::PlayMode::Normal;
    return;
  }

  command.second = {};
  clearStreamBuffers();
  if(trackID) {
    cdda.playTrack = trackID;
    cdda.holdLBA = drive.lba.request;
  }
  if(drive.lba.pending) {
    drive.seekType = Drive::SeekType::SeekP;
    drive.pendingOperation = Drive::PendingOperation::Play;
    drive.beginSeek();
  } else {
    drive.beginStream(Drive::PendingOperation::Play);
  }
}

//0x04
auto Disc::commandForward() -> void {
  if(!canReadMedia() || !ssr.playingCDDA || !cdda.started) return error(ErrorCode_CannotRespondYet);
  cdda.playMode = CDDA::PlayMode::FastForward;
  cdda.scanStep = min(max(s32(cdda.scanStep), 0) + 4, 12);

  ack();
}

//0x05
auto Disc::commandBackward() -> void {
  if(!canReadMedia() || !ssr.playingCDDA || !cdda.started) return error(ErrorCode_CannotRespondYet);
  cdda.playMode = CDDA::PlayMode::Rewind;
  cdda.scanStep = max(min(s32(cdda.scanStep), 0) - 4, -12);

  ack();
}

//0x06
auto Disc::commandReadN() -> void {
  if(!canReadMedia()) return error(ErrorCode_CannotRespondYet);
  if(!drive.mode.cdda && (audioCD() || !discRegionMatches())) return error(ErrorCode_InvalidCommand);
  ack();
  bool continuing = (ssr.reading && !ssr.playingCDDA) || drive.pendingOperation == Drive::PendingOperation::Read;
  s32 next = drive.seeking ? drive.lba.seek : drive.lba.current;
  if(continuing && (!drive.lba.pending || drive.lba.request == next)) {
    drive.lba.pending = 0;
    return;
  }
  if(!drive.lba.pending) {
    if(drive.resetDelay && drive.lba.hold) {
      // Reference Read attaches to the reset's implicit seek, preserving its deadline and quiet status.
      drive.seeking = drive.resetDelay;
      drive.resetDelay = 0;
      drive.lba.seek = 0;
      drive.seekType = Drive::SeekType::SeekL;
      drive.seekVisible = false;
    }
    if(drive.seeking) {
      drive.pendingOperation = Drive::PendingOperation::Read;
      return;
    }
  }

  command.second = {};
  clearStreamBuffers();
  if(drive.lba.pending) {
    drive.seekType = Drive::SeekType::SeekL;
    drive.pendingOperation = Drive::PendingOperation::Read;
    drive.beginSeek();
  } else {
    drive.beginStream(Drive::PendingOperation::Read);
  }
}

auto Disc::stopDriveActivity() -> void {
  drive.activeStatusCleared = false;
  ssr.reading = 0;
  ssr.playingCDDA = 0;
  drive.seeking = drive.speedChange = 0;
  drive.seekVisible = drive.streamStarting = false;
  drive.seekType = Drive::SeekType::None;
  drive.pendingOperation = Drive::PendingOperation::None;
  drive.sessionChange = {};
  drive.spinUp = 0;
  drive.shellOpening = 0;
  drive.resetDelay = 0;
}

auto Disc::startMotor() -> void {
  if(ssr.motorOn) return;
  ssr.motorOn = 1;
  drive.spinUp = system.frequency();
}

auto Disc::resetAudioDecoder() -> void {
  audio.sample = {};
  cdda.autoPausePending = false;
  cdxa.current = {};
  audio.frames.flush();
  for(auto& value : cdxa.previousSamples) value = 0;
  for(auto& channel : cdxa.resampleRing) for(auto& value : channel) value = 0;
  cdxa.resamplePosition = 0;
  cdxa.resampleStep = 6;
}

auto Disc::completeStatusResponse() -> void {
  if(!canReadMedia()) {
    queueResponse(ResponseType::Error, {u8(status() | 0x01), ErrorCode_DoorOpen}, true);
  } else {
    queueResponse(ResponseType::Complete, {status()});
  }
}

//0x07
auto Disc::commandMotorOn(bool secondResponse) -> void {
  if(secondResponse) return completeStatusResponse();
  if(ssr.motorOn) return error(ErrorCode_InvalidParameterCount);
  if(!canReadMedia()) return error(ErrorCode_CannotRespondYet);

  ack();
  if(command.second.pending && command.second.command == 0x07) return;
  stopDriveActivity();
  startMotor();
  scheduleSecondResponse(0x07, 400'000);
}

//0x08
auto Disc::commandStop(bool secondResponse) -> void {
  if(secondResponse) return completeStatusResponse();
  u32 delay = ssr.motorOn ? (drive.mode.speed ? 25'000'000 : 13'000'000) : 7000;
  fifo.deferred = {};
  command.second = {};
  ack();  // Reference returns the pre-stop status, unlike the H2 description's cleared read bit.
  stopDriveActivity();
  ssr.motorOn = 0;
  drive.setHold(0, 0);
  drive.headerValid = false;
  scheduleSecondResponse(0x08, delay);
}

//0x09
auto Disc::commandPause(bool secondResponse) -> void {
  if(secondResponse) return completeStatusResponse();
  u32 delay = drive.pauseDelay();
  if(ssr.reading && (drive.sector.subq[0] & 0x40)) {
    s32 track = drive.sectorsPerTrack(drive.lba.hold);
    drive.setHold(drive.lba.hold, max(0, drive.lba.hold - track));
  }
  command.second = {};
  if(drive.seeking || drive.seekType != Drive::SeekType::None || (drive.streamStarting && (status() & 0x40))) {
    // H2 specifies INT5(stat,80h). Reference also leaves an earlier ACK byte in its FIFO.
    return error(ErrorCode_CannotRespondYet);
  }

  ack();
  fifo.deferred = {};
  drive.lba.current = drive.lba.hold;
  stopDriveActivity();
  resetAudioDecoder();
  scheduleSecondResponse(0x09, delay);
}

auto Disc::clearSectorBuffers() -> void {
  fifo.data.flush();
  for(auto& sector : fifo.sectors) sector.flush();
  fifo.sectorRead = fifo.sectorWrite = 0;
  fifo.sectorPending = fifo.hostLoaded = false;
  fifo.sectorFile = fifo.sectorChannel = 0;
  fifo.sectorNeedsFilter = false;
  io.sectorBufferReadRequest = 0;
}

auto Disc::clearStreamBuffers() -> void {
  fifo.deferred = {};
  clearSectorBuffers();
  resetAudioDecoder();
  cdda.scanStep = 0;
  cdda.started = false;
  cdda.playTrack = 0;
  cdda.holdLBA = drive.lba.hold;
  updateCommandEvent();
}

auto Disc::softReset() -> u32 {
  bool doubleSpeed = drive.mode.speed;
  if(drive.seeking || (drive.resetDelay && drive.lba.hold)) drive.updateSeekPosition();
  else drive.updatePosition();
  command.second = {};
  stopDriveActivity();
  ssr = {};
  ssr.motorOn = canReadMedia();
  ssr.shellOpen = lidOpen;
  drive.mode = {};
  drive.mode.sectorSize = 1;
  drive.lba.request = 0;
  drive.lba.pending = 0;
  io.sectorBufferReadRequest = io.sectorBufferWriteRequest = io.soundMapEnable = 0;
  fifo.deferred = {};
  fifo.parameter.flush();
  clearSectorBuffers();
  drive.headerValid = false;
  resetAudioDecoder();
  audio.mute = audio.muteADPCM = 0;

  u32 delay = 4'000'000;
  if(canReadMedia()) {
    // Speed delay is reference-derived. Seek geometry remains the shared mechanical model (DISC-019/032).
    u32 speedDelay = doubleSpeed ? system.frequency() * 7 / 10 : 0;
    u32 seekDelay = drive.lba.hold ? drive.distance(0) : 0;
    delay = max(delay, speedDelay + seekDelay);
    drive.resetDelay = delay;
    drive.lba.start = drive.lba.hold;
    drive.seekInterval = delay;
  }
  return delay;
}

//0x0a
auto Disc::commandInit(bool secondResponse) -> void {
  if(secondResponse) return completeStatusResponse();
  if(command.second.pending && command.second.command == 0x0a) return;
  ack();
  scheduleSecondResponse(0x0a, softReset());
}

//0x0b
auto Disc::commandMute() -> void {
  audio.mute = 1;

  ack();
}

//0x0c
auto Disc::commandDemute() -> void {
  audio.mute = 0;

  ack();
}

//0x0d
auto Disc::commandSetFilter() -> void {
  cdxa.filter.file = fifo.parameter.read(0);
  cdxa.filter.channel = fifo.parameter.read(0);
  cdxa.current.selected = false;

  ack();
}

//0x0e
auto Disc::commandSetMode() -> void {
  if(fifo.parameter.size() != 1) {
    error(ErrorCode_InvalidParameterCount);
    return;
  }

  n8 data = fifo.parameter.read(0);

  drive.changeSpeed(data.bit(7));

  drive.mode.cdda       = data.bit(0);
  drive.mode.autoPause  = data.bit(1);
  drive.mode.report     = data.bit(2);
  drive.mode.xaFilter   = data.bit(3);
  drive.mode.ignore     = data.bit(4);  // Stored/reported only: reference fallback for unverified Ignore semantics.
  drive.mode.sectorSize = data.bit(5);
  drive.mode.xaADPCM    = data.bit(6);
  drive.mode.speed      = data.bit(7);

  ack();
}

//0x0f
auto Disc::commandGetParam() -> void {
  queueResponse(ResponseType::Acknowledge, {
    status(),
    mode(),
    0,
    cdxa.filter.file,
    cdxa.filter.channel
  });
}

//0x10
auto Disc::commandGetlocL() -> void {
  if(!drive.headerValid) return error(ErrorCode_CannotRespondYet);
  drive.updatePosition(true);
  queueResponse(ResponseType::Acknowledge, {
    drive.header[0], drive.header[1], drive.header[2], drive.header[3],
    drive.header[4], drive.header[5], drive.header[6], drive.header[7]
  });
}

//0x11
auto Disc::commandGetlocP() -> void {
  if(!canReadMedia()) return error(ErrorCode_CannotRespondYet);
  if(drive.seeking || (drive.resetDelay && drive.lba.hold)) drive.updateSeekPosition();
  else drive.updatePosition();
  drive.updateSubQ();
  queueResponse(ResponseType::Acknowledge, {
    drive.sector.subq[1], //track
    drive.sector.subq[2], //index
    drive.sector.subq[3], //mm
    drive.sector.subq[4], //ss
    drive.sector.subq[5], //sect/frame
    drive.sector.subq[7], //amm
    drive.sector.subq[8], //ass
    drive.sector.subq[9], //asect/frame
  });
}

//0x12
auto Disc::commandSetSession(bool secondResponse) -> void {
  if(!secondResponse) {
    u8 number = fifo.parameter.read(0);
    if(!canReadMedia() || ssr.reading || ssr.playingCDDA) return error(ErrorCode_CannotRespondYet);
    if(!number) return error(ErrorCode_InvalidParameterValue);

    command.second = {};
    ack();
    drive.seeking = drive.speedChange = 0;
    drive.seekVisible = drive.streamStarting = false;
    drive.seekType = Drive::SeekType::None;
    drive.pendingOperation = Drive::PendingOperation::None;
    // Changing session is a drive event, independent of a later command's second response.
    drive.spinUp = 0;
    drive.resetDelay = 0;
    drive.sessionChange = {number, system.frequency() / 2, true};
    drive.activeStatusCleared = false;
    return;
  }

  ssr.reading = 0;
  ssr.playingCDDA = 0;
  ssr.motorOn = 1;
  if(drive.sessionChange.number == 1) {
    queueResponse(ResponseType::Complete, {status()});
  } else {
    // Reference compatibility: only session 1 is supported; the seek error is response-local.
    queueResponse(ResponseType::Error, {u8(status() | 0x04), 0x40}, true);
  }
}

//0x13
auto Disc::commandGetTN() -> void {
  if(!canReadMedia()) return error(ErrorCode_CannotRespondYet);
  if(!fifo.parameter.empty()) {
    error(ErrorCode_InvalidParameterCount);
    return;
  }

  queueResponse(ResponseType::Acknowledge, {
    status(),
    BCD::encode(session.firstTrack),
    BCD::encode(session.lastTrack)
  });
}

//0x14
auto Disc::commandGetTD() -> void {
  if(!canReadMedia()) return error(ErrorCode_CannotRespondYet);
  if(fifo.parameter.size() != 1) {
    error(ErrorCode_InvalidParameterCount);
    return;
  }

  if(!BCD::valid(fifo.parameter.peek(0))) {
    error(ErrorCode_InvalidParameterValue);
    return;
  }

  u8 trackID = BCD::decode(fifo.parameter.read(0));
  s32 lba = session.leadOut.lba;
  if(trackID) {
    auto track = session.track(trackID);
    if(!track) return error(ErrorCode_InvalidParameterValue);
    auto index = track->index(1);
    if(!index) return error(ErrorCode_InvalidParameterValue);
    lba = index->lba;
  }
  auto msf = CD::MSF::fromABA(CD::LBAtoABA(lba));

  queueResponse(ResponseType::Acknowledge, {
    status(),
    BCD::encode(msf.minute),
    BCD::encode(msf.second)
  });
}

//0x15
auto Disc::commandSeekL() -> void {
  if(!canReadMedia()) return error(ErrorCode_CannotRespondYet);
  if(!fifo.parameter.empty()) { error(ErrorCode_InvalidParameterCount); return; }

  command.second = {};
  clearStreamBuffers();
  ack();

  drive.seekType = Drive::SeekType::SeekL;
  drive.pendingOperation = Drive::PendingOperation::None;
  drive.beginSeek();
  drive.lba.pending = 0;
}

//0x16
auto Disc::commandSeekP() -> void {
  if(!canReadMedia()) return error(ErrorCode_CannotRespondYet);
  if(!fifo.parameter.empty()) { error(ErrorCode_InvalidParameterCount); return; }

  command.second = {};
  clearStreamBuffers();
  ack();

  drive.seekType = Drive::SeekType::SeekP;
  drive.pendingOperation = Drive::PendingOperation::None;
  drive.beginSeek();
  drive.lba.pending = 0;
}

//0x19 0x04
auto Disc::commandTestStartReadSCEX() -> void {
  ssr.motorOn = 1;
  ack();
}

//0x19 0x05
auto Disc::commandTestStopReadSCEX() -> void {
  // Report no SCEX string found to appease mod-chip detection
  queueResponse(ResponseType::Acknowledge, {0, 0});
}

//0x19 0x20
auto Disc::setControllerVersion(string name) -> bool {
  for(u32 index : range(std::size(controllerProfiles))) {
    if(name != controllerProfiles[index].name) continue;
    controllerVersion = index;
    return true;
  }
  return false;
}

auto Disc::commandTestControllerDate() -> void {
  if(controllerVersion >= std::size(controllerProfiles)) return error(ErrorCode_InvalidCommand);
  auto& date = controllerProfiles[controllerVersion].date;
  queueResponse(ResponseType::Acknowledge, {date[0], date[1], date[2], date[3]});
}

auto Disc::discRegionSuffix() const -> u8 {
  // MIA's multi-region fallback does not establish an SCEx license.
  if(region() == "NTSC-J") return 'I';
  if(region() == "NTSC-U") return 'A';
  if(region() == "PAL") return 'E';
  return 0;
}

auto Disc::discRegionMatches() const -> bool {
  auto suffix = discRegionSuffix();
  if(controllerVersion == 4) return true;  // D1 accepts unlicensed data media.
  if(controllerVersion == 6) return suffix != 0;  // Yaroze accepts licensed regions, not generic unlicensed images.
  return (suffix == 'I' && Region::NTSCJ()) || (suffix == 'A' && Region::NTSCU())
    || (suffix == 'E' && Region::PAL());
}

//0x1a
auto Disc::commandGetID(bool secondResponse) -> void {
  if(!secondResponse) {
    command.second = {};
    // H2 distinguishes closed-empty from open, spindle startup and TOC settling.
    if(lidOpen || drive.shellOpening || drive.spinUp || drive.resetDelay || drive.speedChange) {
      return error(ErrorCode_CannotRespondYet);
    }
    scheduleSecondResponse(0x1a, 33'868);  // Reference ID_READ_TICKS, not measured firmware timing.
    ack();
    return;
  }

  // Reference GetID clears visible activity without cancelling the independent drive event.
  drive.activeStatusCleared = true;
  ssr.motorOn = canReadMedia();

  u8 flags = 0;
  u8 type = noDisc() || audioCD() ? 0 : session.format;
  u8 scex = !noDisc() && !audioCD() ? discRegionSuffix() : 0;
  if(noDisc()) {
    flags = 0x40;
  } else if(audioCD()) {
    flags = 0x90;
  } else if(!discRegionMatches()) {
    flags = 0x80;
    if(!scex) {
      for(auto& track : session.tracks) {
        if(track && track.isAudio()) flags |= 0x10;
      }
    }
  }

  // The ID failure bit belongs to this response; it is not a persistent controller error.
  u8 responseStatus = (status() & ~0x08) | (flags ? 0x08 : 0);
  bool spaces = !flags && (controllerVersion == 4 || (controllerVersion == 6 && scex != 'A'));
  queueResponse(flags ? ResponseType::Error : ResponseType::Complete, {
    responseStatus, flags, type, 0,
    u8(spaces ? ' ' : scex ? 'S' : 0), u8(spaces ? ' ' : scex ? 'C' : 0),
    u8(spaces ? ' ' : scex ? 'E' : 0), u8(spaces ? ' ' : scex)
  }, true);
}

//0x1b
auto Disc::commandReadS() -> void {
  // Reference fallback: both read commands report image I/O failure; hardware ECC/retry differences remain unknown.
  return commandReadN();
}

//0x1e
auto Disc::commandReadToc(bool secondResponse) -> void {
  if(!secondResponse) {
    if(controllerVersion < 2) return error(ErrorCode_InvalidCommand);
    command.second = {};
    if(!canReadMedia()) return error(ErrorCode_CannotRespondYet);
    scheduleSecondResponse(0x1e, system.frequency() / 2);

    ack();
    return;
  }

  if(secondResponse) {
    completeStatusResponse();
    return;
  }
}
