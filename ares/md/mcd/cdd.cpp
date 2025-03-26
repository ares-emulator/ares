//NEC uPD75006 (G-631)
//4-bit MCU HLE

auto MCD::CDD::load(Node::Object parent) -> void {
  dac.load(parent);
}

auto MCD::CDD::unload(Node::Object parent) -> void {
  dac.unload(parent);
}

auto MCD::CDD::clock() -> void {
  if(++counter < 434) return;
  counter = 0;

  //75hz
  if(!hostClockEnable) return;

  if(statusPending) {
    statusPending = 0;
    irq.raise();
  }

  switch(io.status) {

  case Status::Stopped: {
    io.status = mcd.fd ? Status::ReadingTOC : Status::NoDisc;
    io.sector = 0;
    io.sample = 0;
    io.track  = 0;
  } break;

  case Status::ReadingTOC: {
    io.sector++;
    if(!session.inLeadIn(io.sector)) {
      io.status = Status::Paused;
      if(auto track = session.inTrack(io.sector)) io.track = track();
      io.tocRead = 1;
    }
  } break;

  case Status::Playing: {
    readSubcode();
    if(session.tracks[io.track].isAudio()) break;
    mcd.cdc.decode(io.sector);
    advance();
  } break;

  case Status::Tracking: [[fallthrough]]; // TODO: implement tracking logic
  case Status::Seeking: {
    if(io.latency && --io.latency) break;
    io.status = io.seeking;
    if(auto track = session.inTrack(io.sector)) io.track = track();
  } break;

  }
}

auto MCD::CDD::advance() -> void {
  if(auto track = session.inTrack(io.sector + 1)) {
    io.track = track();
    io.sector++;
    io.sample = 0;
    return;
  }

  io.status = Status::LeadOut;
  io.track = 0xaa;
}

auto MCD::CDD::sample() -> void {
  i16 left  = 0;
  i16 right = 0;
  if(io.status == Status::Playing) {
    if(session.tracks[io.track].isAudio()) {
      mcd.fd->seek((abs(session.leadIn.lba) + io.sector) * 2448 + io.sample);
      left  = mcd.fd->readl(2);
      right = mcd.fd->readl(2);
      io.sample += 4;
      if(io.sample >= 2352) advance();
    }
  }
  dac.sample(left, right);
}

//convert sector# to normalized sector position on the CD-ROM surface for seek latency calculation
auto MCD::CDD::position(s32 sector) -> double {
  static const f64 sectors = 7500.0 + 330000.0 + 6750.0;
  static const f64 radius = 0.058 - 0.024;
  static const f64 innerRadius = 0.024 * 0.024;  //in mm
  static const f64 outerRadius = 0.058 * 0.058;  //in mm

  sector += session.leadIn.lba;  //convert to natural
  return sqrt(sector / sectors * (outerRadius - innerRadius) + innerRadius) / radius;
}

auto MCD::CDD::readSubcode() -> void {
  if(!mcd.fd) return;

  mcd.fd->seek(((abs(mcd.cdd.session.leadIn.lba) + io.sector) * 2448) + 2352);
  vector<u8> subchannel;
  subchannel.resize(96);
  mcd.fd->read({subchannel.data(), 96});

  io.subcodePosition += 49;
  n6 index = io.subcodePosition;

  //subchannel data is stored by ares as 12x8 for each channel (PQRSTUVW)
  //we need to convert this back to the raw subchannel encoding as per EMCA-130
  for(int i = 0; i < 96; i+= 2) {
    n16 data = 0;
    for(auto channel : range(8)) {
      n8 input = subchannel[(channel * 12) + (i / 8)];
      n2 bits = input >> (6 - (i & 6));
      data.bit( 7 - channel) = bits.bit(0);
      data.bit(15 - channel) = bits.bit(1);
    }

    subcode[index++] = data;
  }

  mcd.irq.subcode.raise();
}

auto MCD::CDD::process() -> void {
//if(command[0]) print("CDD ", command[0], ":", command[3], "\n");

  if(!valid()) {
    //unverified
    debug(unusual, "[MCD::CDD::process] CDD checksum error");
    io.status = Status::ChecksumError;
  }

  else
  switch(command[0]) {

  case Command::Idle: {
    //fixes Lunar: The Silver Star
    if(!io.latency && status[1] == 0xf) {
      status[1] = 0x2;
      status[2] = io.track / 10; status[3] = io.track % 10;
    }
  } break;

  case Command::Stop: {
    stop();
    status[1] = 0x0;
    status[2] = 0x0; status[3] = 0x0;
    status[4] = 0x0; status[5] = 0x0;
    status[6] = 0x0; status[7] = 0x0;
    status[8] = 0x0;
  } break;

  case Command::Request: {
    switch(command[3]) {

    case Request::AbsoluteTime: {
      auto [minute, second, frame] = CD::MSF(io.sector);
      status[1] = command[3];
      status[2] = minute / 10; status[3] = minute % 10;
      status[4] = second / 10; status[5] = second % 10;
      status[6] = frame  / 10; status[7] = frame  % 10;
      status[8] = session.tracks[io.track].isData() << 2;
    } break;

    case Request::RelativeTime: {
      auto [minute, second, frame] = CD::MSF(io.sector - session.tracks[io.track].indices[1].lba);
      status[1] = command[3];
      status[2] = minute / 10; status[3] = minute % 10;
      status[4] = second / 10; status[5] = second % 10;
      status[6] = frame  / 10; status[7] = frame  % 10;
      status[8] = session.tracks[io.track].isData() << 2;
    } break;

    case Request::TrackInformation: {
      status[1] = command[3];
      status[2] = io.track / 10; status[3] = io.track % 10;
      status[4] = 0x0; status[5] = 0x0;
      status[6] = 0x0; status[7] = 0x0;
      status[8] = 0x0;
      if(session.inLeadIn (io.sector)) { status[2] = 0x0; status[3] = 0x0; }
      if(session.inLeadOut(io.sector)) { status[2] = 0xa; status[3] = 0xa; }
    } break;

    case Request::DiscCompletionTime: {  //time in mm:ss:ff
      auto [minute, second, frame] = CD::MSF(session.leadOut.lba);
      status[1] = command[3];
      status[2] = minute / 10; status[3] = minute % 10;
      status[4] = second / 10; status[5] = second % 10;
      status[6] = frame  / 10; status[7] = frame  % 10;
      status[8] = 0x0;
    } break;

    case Request::DiscTracks: {  //first and last track numbers
      auto firstTrack = session.firstTrack;
      auto lastTrack  = session.lastTrack;
      status[1] = command[3];
      status[2] = firstTrack / 10; status[3] = firstTrack % 10;
      status[4] = lastTrack  / 10; status[5] = lastTrack  % 10;
      status[6] = 0x0; status[7] = 0x0;
      status[8] = 0x0;
    } break;

    case Request::TrackStartTime: {
      if(command[4] > 9 || command[5] > 9) break;
      u32 track  = command[4] * 10 + command[5];
      auto [minute, second, frame] = CD::MSF(session.tracks[track].indices[1].lba);
      status[1] = command[3];
      status[2] = minute / 10; status[3] = minute % 10;
      status[4] = second / 10; status[5] = second % 10;
      status[6] = frame  / 10; status[7] = frame  % 10;
      status[6].bit(3) = session.tracks[track].isData();
      status[8] = track % 10;
    } break;

    case Request::ErrorInformation: {
      //always report no errors
      status[1] = command[3];
      status[2] = 0x0; status[3] = 0x0;
      status[4] = 0x0; status[5] = 0x0;
      status[6] = 0x0; status[7] = 0x0;
      status[8] = 0x0;
    } break;

    }
  } break;

  case Command::SeekPlay: {
    u32 minute = command[2] * 10 + command[3];
    u32 second = command[4] * 10 + command[5];
    u32 frame  = command[6] * 10 + command[7];
    s32 lba    = minute * 60 * 75 + second * 75 + frame - 3;

    counter    = 0;
    io.status  = Status::Seeking;
    io.seeking = Status::Playing;
    io.latency = 11 + 112.5 * abs(position(io.sector) - position(lba));
    io.sector  = lba;
    io.sample  = 0;

    status[1] = 0xf;
    status[2] = 0x0; status[3] = 0x0;
    status[4] = 0x0; status[5] = 0x0;
    status[6] = 0x0; status[7] = 0x0;
    status[8] = 0x0;
  } break;

  case Command::SeekPause: {
    u32 minute = command[2] * 10 + command[3];
    u32 second = command[4] * 10 + command[5];
    u32 frame  = command[6] * 10 + command[7];
    s32 lba    = minute * 60 * 75 + second * 75 + frame - 3;

    counter    = 0;
    io.status  = Status::Seeking;
    io.seeking = Status::Paused;
    io.latency = 11 + 112.5 * abs(position(io.sector) - position(lba));
    io.sector  = lba;
    io.sample  = 0;

    status[1] = 0xf;
    status[2] = 0x0; status[3] = 0x0;
    status[4] = 0x0; status[5] = 0x0;
    status[6] = 0x0; status[7] = 0x0;
    status[8] = 0x0;
  } break;

  case Command::Pause: {
    pause();
  } break;

  case Command::Play: {
    play();
  } break;

  // TrackSkip command directs movement of the laser/sled by a specified radial track count.
  // Certain versions of the Mega-CD BIOS will issue this tracking command followed by a
  // Seek command (buffered). Therefore, it is not critical to handle the operation explicitly.
  // Actual seek algorithms (implemented in the MCU program) are currently unknown.
  case Command::TrackSkip: {
    // cmd[2..3] : Direction (flag)
    //   non-zero == inward movement (i.e., backtracking); zero == outward movement
    // cmd[4..7] : Magnitude (in hexadecimal -- not BCD)
    //   number of radial tracks to traverse

    //u8  direction = command[2] << 4 | command[3];
    //s16 magnitude = command[4] << 12 | command[5] << 8 | command[6] << 4 | command[7];
    //magnitude = direction ? -magnitude : magnitude;

    // TODO: Implement this properly. For now, it is treated as a null seek operation.

    counter    = 0;
    io.status  = Status::Tracking;
    io.seeking = Status::Paused;
    io.latency = 0;
    //io.sector  = lba;
    io.sample  = 0;

    status[1] = 0xf;
    status[2] = 0x0; status[3] = 0x0;
    status[4] = 0x0; status[5] = 0x0;
    status[6] = 0x0; status[7] = 0x0;
    status[8] = 0x0;
  } break;

  // TrackCue command directs movement of the laser/sled to find a target track index number.
  // It is not known to be used by the CD BIOS, so it remains unsupported.
  //case Command::TrackCue: {
  //} break;

  default:
    io.status = Status::CommandError;
  }

  status[0] = io.status;
  checksum();
  statusPending = 1;
}

auto MCD::CDD::valid() -> bool {
  n4 checksum;
  for(u32 index : range(9)) checksum += command[index];
  checksum = ~checksum;
  return checksum == command[9];
}

auto MCD::CDD::checksum() -> void {
  n4 checksum;
  for(u32 index : range(9)) checksum += status[index];
  checksum = ~checksum;
  status[9] = checksum;
}

auto MCD::CDD::insert() -> void {
  if(!mcd.disc || !mcd.fd) {
    io.status = Status::NoDisc;
    return;
  }

  //read TOC (table of contents) from disc lead-in
  u32 sectors = mcd.fd->size() / 2448;
  vector<u8> subchannel;
  subchannel.resize(sectors * 96);
  for(u32 sector : range(sectors)) {
    mcd.fd->seek(sector * 2448 + 2352);
    mcd.fd->read({subchannel.data() + sector * 96, 96});
  }
  session.decode(subchannel, 96);

  io.status  = Status::ReadingTOC;
  io.sector  = session.leadIn.lba;
  io.sample  = 0;
  io.track   = 0;
}

auto MCD::CDD::eject() -> void {
  session = {};

  io.status  = Status::NoDisc;
  io.sector  = 0;
  io.sample  = 0;
  io.track   = 0;
}

auto MCD::CDD::stop() -> void {
  io.status = mcd.fd ? Status::Stopped : Status::NoDisc;
}

auto MCD::CDD::play() -> void {
  io.status = Status::Playing;
}

auto MCD::CDD::pause() -> void {
  io.status = Status::Paused;
}

auto MCD::CDD::seekToTime(u8 minute, u8 second, u8 frame, bool startPaused) -> void {
  s32 lba = (s32)minute * 60 * 75 + (s32)second * 75 + (s32)frame - 3;
  seekToSector(lba, startPaused);
}

auto MCD::CDD::seekToRelativeTime(n7 track, u8 minute, u8 second, u8 frame, bool startPaused) -> void {
  auto firstTrack = session.firstTrack;
  auto lastTrack = session.lastTrack;
  if ((track < firstTrack) || (track > lastTrack)) {
    return;
  }

  auto targetTrackLba = session.tracks[track].indices[1].lba;
  s32 lba = targetTrackLba + ((s32)minute * 60 * 75 + (s32)second * 75 + (s32)frame - 3);
  seekToSector(lba, startPaused);
}

auto MCD::CDD::seekToSector(s32 lba, bool startPaused) -> void {
  counter = 0;
  io.status = Status::Seeking;
  io.seeking = (startPaused ? Status::Paused : Status::Playing);
  io.latency = 11 + 112.5 * abs(position(io.sector) - position(lba));
  io.sector = lba;
  io.sample = 0;
  if (auto track = session.inTrack(io.sector)) io.track = track();
}

auto MCD::CDD::seekToTrack(n7 track, bool startPaused) -> void {
  auto lba = session.tracks[track].indices[1].lba;
  seekToSector(lba, startPaused);
}

auto MCD::CDD::getTrackCount() -> n7 {
  auto lastTrack = session.lastTrack;
  return lastTrack;
}

auto MCD::CDD::getCurrentTrack() -> n7 {
  return io.track;
}

auto MCD::CDD::getCurrentSector() -> s32 {
  return io.sector;
}

auto MCD::CDD::getCurrentTimecode(u8& minute, u8& second, u8& frame) -> void {
  auto [lminute, lsecond, lframe] = CD::MSF(io.sector);
  minute = lminute;
  second = lsecond;
  frame = lframe;
}

auto MCD::CDD::getCurrentTrackRelativeTimecode(u8& minute, u8& second, u8& frame) -> void {
  auto [lminute, lsecond, lframe] = CD::MSF(io.sector - session.tracks[io.track].indices[1].lba);
  minute = lminute;
  second = lsecond;
  frame = lframe;
}

auto MCD::CDD::getLeadOutTimecode(u8& minute, u8& second, u8& frame) -> void {
  auto [lminute, lsecond, lframe] = CD::MSF(session.leadOut.lba);
  minute = lminute;
  second = lsecond;
  frame = lframe;
}

auto MCD::CDD::getTrackTocData(n7 track, u8& flags, u8& minute, u8& second, u8& frame) -> void {
  auto [lminute, lsecond, lframe] = CD::MSF(session.tracks[track].indices[1].lba);
  minute = lminute;
  second = lsecond;
  frame = lframe;
  flags = session.tracks[track].control;
}

auto MCD::CDD::power(bool reset) -> void {
  irq = {};
  counter = 0;
  dac.power(reset);
  io = {};
  hostClockEnable = 0;
  statusPending = 0;
  for(auto& data : status) data = 0x0;
  for(auto& data : command) data = 0x0;
  insert();
  checksum();
}
