//computes the number of 75hz intervals required to seek to a given sector.
//the logarithm is based on observed results from a TurboDuo, and errs on seeking too fast.
auto PCD::Drive::distance() -> uint {
  auto distance = abs(lba - start);
  return 17 + pow(sqrt(distance), 0.99) * 0.3;
}

auto PCD::Drive::seekRead() -> void {
  mode = Mode::Seeking;
  seek = Mode::Reading;
  latency = distance();
}

auto PCD::Drive::seekPlay() -> void {
  mode = Mode::Seeking;
  seek = Mode::Playing;
  latency = distance();
}

auto PCD::Drive::seekPause() -> void {
  mode = Mode::Seeking;
  seek = Mode::Paused;
  latency = distance();
}

auto PCD::Drive::read() -> bool {
  if(seeking()) {
    if(latency && --latency) return false;
    mode = seek;
    lba = start;
  }

//print("* ", reading() ? "data" : "cdda", " read ", lba, " to ", end - 1, "\n");

  pcd.fd->seek(2448 * (abs(session->leadIn.lba) + lba));
  pcd.fd->read(sector, 2448);
  if(++lba == end) setInactive();
  return true;
}

auto PCD::Drive::power() -> void {
  mode    = Mode::Inactive;
  seek    = Mode::Inactive;
  latency = 0;
  lba     = 0;
  start   = 0;
  end     = 0;
}
