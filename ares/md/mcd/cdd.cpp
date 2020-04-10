//NEC uPD75006 (G-631)
//4-bit MCU HLE

auto MCD::CDD::load(Node::Object parent) -> void {
  dac.load(parent);
}

auto MCD::CDD::unload() -> void {
  dac.unload();
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
    io.status = mcd.disc ? Status::ReadingTOC : Status::NoDisc;
    io.sector = 0;
    io.sample = 0;
    io.track  = 0;
  } break;

  case Status::ReadingTOC: {
    io.sector++;
    if(!session.inLeadIn(io.sector)) {
      io.status = Status::Paused;
      if(auto track = session.inTrack(io.sector)) io.track = track();
    }
  } break;

  case Status::Playing: {
    if(session.tracks[io.track].isAudio()) break;
    mcd.cdc.decode(io.sector);
    advance();
  } break;

  case Status::Seeking: {
    if(io.latency && --io.latency) break;
    io.status = io.seeking;
    if(auto track = session.inTrack(io.sector)) io.track = track();
  } break;

  }
}

auto MCD::CDD::advance() -> void {
  //is the next sector still within the current track?
  if(auto track = session.inTrack(io.sector + 1)) {
    if(track() == io.track) {
      io.sector++;
      io.sample = 0;
      return;
    }
  }

  //if not, then the track has been completed
  io.status = Status::Paused;
}

auto MCD::CDD::sample() -> void {
  int16 left  = 0;
  int16 right = 0;
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
auto MCD::CDD::position(int sector) -> double {
  static const double sectors = 7500.0 + 330000.0 + 6750.0;
  static const double radius = 0.058 - 0.024;
  static const double innerRadius = 0.024 * 0.024;  //in mm
  static const double outerRadius = 0.058 * 0.058;  //in mm

  sector += session.leadIn.lba;  //convert to natural
  return sqrt(sector / sectors * (outerRadius - innerRadius) + innerRadius) / radius;
}

auto MCD::CDD::process() -> void {
//if(command[0]) print("CDD ", command[0], ":", command[3], "\n");

  if(!valid()) {
    //unverified
    print("CDD checksum error\n");
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
    io.status = Status::Stopped;
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
      uint track  = command[4] * 10 + command[5];
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
    uint minute = command[2] * 10 + command[3];
    uint second = command[4] * 10 + command[5];
    uint frame  = command[6] * 10 + command[7];
     int lba    = minute * 60 * 75 + second * 75 + frame - 3;

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
    uint minute = command[2] * 10 + command[3];
    uint second = command[4] * 10 + command[5];
    uint frame  = command[6] * 10 + command[7];
     int lba    = minute * 60 * 75 + second * 75 + frame - 3;

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
    io.status = Status::Paused;
  } break;

  case Command::Play: {
    io.status = Status::Playing;
  } break;

  }

  status[0] = io.status;
  checksum();
  statusPending = 1;
}

auto MCD::CDD::valid() -> bool {
  uint4 checksum;
  for(uint index : range(9)) checksum += command[index];
  checksum = ~checksum;
  return checksum == command[9];
}

auto MCD::CDD::checksum() -> void {
  uint4 checksum;
  for(uint index : range(9)) checksum += status[index];
  checksum = ~checksum;
  status[9] = checksum;
}

auto MCD::CDD::insert() -> void {
  if(!mcd.disc || !mcd.fd) {
    io.status = Status::NoDisc;
    return;
  }

  uint sectors = mcd.fd->size() / 2448;
  vector<uint8_t> sub;
  sub.resize(sectors * 96);
  for(uint sector : range(sectors)) {
    mcd.fd->seek(sector * 2448 + 2352);
    mcd.fd->read(sub.data() + sector * 96, 96);
  }
  session.decode(sub, 96);

  io.status = Status::ReadingTOC;
  io.sector = session.leadIn.lba;
  io.sample = 0;
  io.track  = 0;
}

auto MCD::CDD::eject() -> void {
  session = {};

  io.status = Status::NoDisc;
  io.sector = 0;
  io.sample = 0;
  io.track  = 0;
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
