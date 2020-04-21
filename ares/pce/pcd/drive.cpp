auto PCD::Drive::read() -> void {
  if(seeking()) mode = seek;

//print("* ", reading() ? "data" : "cdda", " read ", lba, " to ", end - 1, "\n");

  pcd.fd->seek(2448 * (abs(session->leadIn.lba) + lba));
  pcd.fd->read(sector, 2448);
  if(++lba == end) setInactive();
}

auto PCD::Drive::power() -> void {
  mode = Mode::Inactive;
  seek = Mode::Inactive;
  lba = CD::InvalidLBA;
  end = CD::InvalidLBA;
}
