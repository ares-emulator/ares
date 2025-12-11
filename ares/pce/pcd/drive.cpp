//computes the number of 75hz intervals required to seek to a given sector.
//the logarithm is based on observed results from a TurboDuo, and errs on seeking too fast.
auto PCD::Drive::distance() -> u32 {
  if (Model::LaserActive()) {
    //##TODO## Calculate correct latency for the LaserActive
    return 20.0 + 10.0 * abs(position(lba) - position(start));
  }

  auto distance = abs(lba - start);
  return 17 + pow(sqrt(distance), 0.99) * 0.3;
}

//convert sector# to normalized sector position on the CD-ROM surface for seek latency calculation
auto PCD::Drive::position(s32 sector) -> double {
  static const f64 sectors = 7500.0 + 330000.0 + 6750.0;
  static const f64 radius = 0.058 - 0.024;
  static const f64 innerRadius = 0.024 * 0.024;  //in mm
  static const f64 outerRadius = 0.058 * 0.058;  //in mm

  sector += session->leadIn.lba;  //convert to natural
  return sqrt(sector / sectors * (outerRadius - innerRadius) + innerRadius) / radius;
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

  pcd.fd->seek(2448 * (abs(session->leadIn.lba) + lba + 150));
  pcd.fd->read(sector, 2448);

  // Calculate the sector advance amount based on the MegaLD playback modes, if applicable. Note that if a SCSI read
  // command has been issued, the playback modes are ignored, and sectors are advanced linearly until the read is
  // complete. This is based on observed behaviour for Vajra trying to start a game from the title screen.
  i32 sectorAdvanceOffset = 1;
  if (Model::LaserActive() && (end == 0xFFFFFF)) {
    switch (pcd.ld.currentPlaybackMode) {
    case 0x00:
      // Normal playback. Nothing to do.
      sectorAdvanceOffset = 1;
      sectorRepeatCount = 0;
      break;
    case 0x01:
      // Frame skipping. This only affects the analog video update rate, and is handled in MCD::LD::updateCurrentVideoFrameNumber.
      sectorAdvanceOffset = 1;
      sectorRepeatCount = 0;
      break;
    case 0x02:
      // Frame stepping
      switch (pcd.ld.currentPlaybackSpeed) {
      case 0x00:
        //-0x0: 0 frames. This pauses playback in frame stepping mode, and performs a normal playback in frame skipping mode.
        sectorAdvanceOffset = 0;
        sectorRepeatCount = 0;
        break;
      case 0x01:
        //-0x1: 1 frame only. The image will not update after the initial frame. Note that under frame step mode, output register
        // 0x07 will report this step speed as 0x1 only until the single frame step has been performed, after which, the output
        // register will now state a value of 0x0.
        sectorAdvanceOffset = 1;
        sectorRepeatCount = 0;
        break;
      case 0x02:
        //-0x2: 15 FPS instead of 30 (12 seconds for 180 frames) - Display frame 2x
        sectorAdvanceOffset = (++sectorRepeatCount >= 2 ? 1 : 0);
        break;
      case 0x03:
        //-0x3: 7.5 FPS instead of 30 (24 seconds for 180 frames) - Display frame 4x
        sectorAdvanceOffset = (++sectorRepeatCount >= 4 ? 1 : 0);
        break;
      case 0x04:
        //-0x4: 3.75 FPS instead of 30 (24 seconds for 90 frames) - Display frame 8x
        sectorAdvanceOffset = (++sectorRepeatCount >= 8 ? 1 : 0);
        break;
      case 0x05:
        //-0x5: 1.875 FPS instead of 30 (48 seconds for 90 frames) - Display frame 16x
        sectorAdvanceOffset = (++sectorRepeatCount >= 16 ? 1 : 0);
        break;
      case 0x06:
        //-0x6: 1 FPS instead of 30 (45 seconds for 45 frames) - Display frame 30x
        sectorAdvanceOffset = (++sectorRepeatCount >= 30 ? 1 : 0);
        break;
      case 0x07:
        //-0x7: ~0.33r FPS instead of 30 (30 seconds for 10 frames) - Display frame 90x
        sectorAdvanceOffset = (++sectorRepeatCount >= 90 ? 1 : 0);
        break;
      }
      sectorAdvanceOffset = (pcd.ld.currentPlaybackDirection ? (i32)(-sectorAdvanceOffset) : sectorAdvanceOffset);
      sectorRepeatCount = ((pcd.ld.currentPlaybackSpeed >= 0x02) && (sectorAdvanceOffset != 0) ? (i32)0 : sectorRepeatCount);
      break;
    case 0x03:
      // Fast forward
      switch (pcd.ld.currentPlaybackSpeed) {
      case 0x00:
        sectorAdvanceOffset = 1;
        break;
      case 0x01:
        sectorAdvanceOffset = 2;
        break;
      case 0x02:
        sectorAdvanceOffset = 3;
        break;
      case 0x03:
        sectorAdvanceOffset = 8;
        break;
      case 0x04:
        sectorAdvanceOffset = 14;
        break;
      case 0x05:
        sectorAdvanceOffset = 20;
        break;
      case 0x06:
      case 0x07:
        //##TODO## Unimplemented
        //       -0x7/0x06: Search mode. Plays 0.75 seconds of footage forwards in time, with audio, then jumps either forward or back
        //        4 seconds from the resulting point. Note that setting this register to 0x07 is the same as 0x06, but 0x06 is the value
        //        that is reported back as the current mode in either case by output register 0x07.
        break;
      }
      sectorAdvanceOffset = (pcd.ld.currentPlaybackDirection ? (i32)(-sectorAdvanceOffset) : sectorAdvanceOffset);
      sectorRepeatCount = 0;
      break;
    }
  }

  if(auto track = session->inTrack(lba + sectorAdvanceOffset)) {
    this->track = *track;
    lba += sectorAdvanceOffset;

    if(lba == end) {
      setInactive();
    }

    if (Model::LaserActive()) {
      pcd.ld.updateCurrentVideoFrameNumber(lba);

      if (stopPointEnabled && (lba == targetStopPoint)) {
        pcd.ld.handleStopPointReached(lba);
      }
    }
    return true;
  }

  setInactive();
  return true;
}

auto PCD::Drive::power() -> void {
  mode    = Mode::Inactive;
  seek    = Mode::Inactive;
  latency = 0;
  lba     = 0;
  start   = 0;
  end     = 0;
  track = 0;
  stopPointEnabled = false;
  targetStopPoint = 0;
}

auto PCD::Drive::stop() -> void {
  mode = pcd.fd ? Mode::Stopped : Mode::Inactive;
}

auto PCD::Drive::play() -> void {
  if (mode == Mode::Seeking) {
    seek = Mode::Playing;
  } else {
    mode = Mode::Playing;
  }
}

auto PCD::Drive::pause() -> void {
  if (mode == Mode::Seeking) {
    seek = Mode::Paused;
  } else {
    mode = Mode::Paused;
  }
}

auto PCD::Drive::seekToTime(u8 hour, u8 minute, u8 second, u8 frame, bool startPaused) -> void {
  s32 lba = lbaFromTime(hour, minute, second, frame);
  seekToSector(lba, startPaused);
}

auto PCD::Drive::seekToRelativeTime(n7 track, u8 minute, u8 second, u8 frame, bool startPaused) -> void {
  auto firstTrack = session->firstTrack;
  auto lastTrack = session->lastTrack;
  if ((track < firstTrack) || (track > lastTrack)) {
    return;
  }

  auto targetTrackLba = session->tracks[track].indices[1].lba;
  s32 lba = targetTrackLba + ((s32)minute * 60 * 75 + (s32)second * 75 + (s32)frame);
  seekToSector(lba, startPaused);
}

// Note that despite the generic name, this implementation is currently MegaLD specific.
auto PCD::Drive::seekToSector(s32 targetLba, bool startPaused) -> void {
  mode = Mode::Seeking;
  seek = (startPaused ? Mode::Paused : Mode::Playing);
  start = targetLba;
  latency = distance();
  //##DEBUG##
  end = 0xFFFFFF;
  pcd.cdda.sample.offset = 0;
  if (auto trackNo = session->inTrack(lba)) track = *trackNo;
}

auto PCD::Drive::seekToTrack(n7 track, bool startPaused) -> void {
  auto lba = session->tracks[track].indices[0].lba;
  seekToSector(lba, startPaused);
}

auto PCD::Drive::getTrackCount() -> n7 {
  return (session->lastTrack - session->firstTrack) + 1;
}

auto PCD::Drive::getFirstTrack() -> n7 {
  return session->firstTrack;
}

auto PCD::Drive::getLastTrack() -> n7 {
  return session->lastTrack;
}

auto PCD::Drive::getCurrentTrack() -> n7 {
  return track;
}

auto PCD::Drive::getCurrentSector() -> s32 {
  return lba;
}

auto PCD::Drive::getCurrentTimecode(u8& minute, u8& second, u8& frame) -> void {
  auto [lminute, lsecond, lframe] = CD::MSF(lba);
  minute = lminute;
  second = lsecond;
  frame = lframe;
}

auto PCD::Drive::getCurrentTrackRelativeTimecode(u8& minute, u8& second, u8& frame) -> void {
  auto [lminute, lsecond, lframe] = CD::MSF(lba - session->tracks[track].indices[1].lba);
  minute = lminute;
  second = lsecond;
  frame = lframe;
}

auto PCD::Drive::getLeadOutTimecode(u8& minute, u8& second, u8& frame) -> void {
  auto [lminute, lsecond, lframe] = CD::MSF(session->leadOut.lba);
  minute = lminute;
  second = lsecond;
  frame = lframe;
}

auto PCD::Drive::getTrackTocData(n7 track, u8& flags, u8& minute, u8& second, u8& frame) -> void {
  auto [lminute, lsecond, lframe] = CD::MSF(session->tracks[track].indices[1].lba);
  minute = lminute;
  second = lsecond;
  frame = lframe;
  flags = session->tracks[track].control;
}

auto PCD::Drive::lbaFromTime(u8 hour, u8 minute, u8 second, u8 frame) -> s32 {
  s32 lba = ((((((s32)hour * 60) + (s32)minute) * 60) + (s32)second) * 75) + (s32)frame;
  return lba;
}

auto PCD::Drive::isTrackAudio(n7 track) -> bool {
  return session->tracks[track].isAudio();
}

auto PCD::Drive::isDiscLoaded() -> bool {
  return (pcd.disc && pcd.fd);
}

auto PCD::Drive::isDiscLaserdisc() -> bool {
  if (!isDiscLoaded()) {
    return false;
  }
  return laserdiscLoaded;
}

auto PCD::Drive::isLaserdiscClv() -> bool {
  return isDiscLaserdisc() && pcd.ld.video.isCLV;
}

auto PCD::Drive::isLaserdiscDigitalAudioPresent() -> bool {
  return pcd.ld.video.hasDigitalAudio;
}
