// Reference compatibility timing at the normal master clock; no host RNG or speed hacks.
auto Disc::Drive::seekJitter() -> u32 {
  auto rotate = [](u64 value, u32 count) { return value << count | value >> (64 - count); };
  constexpr u64 range = 33'868 - 16'934;
  constexpr u64 limit = (~u64(0) / range) * range;
  while(true) {
    u64 first = seekRandom[0], second = seekRandom[1];
    u64 value = rotate(first + second, 17) + first;
    second ^= first;
    seekRandom[0] = rotate(first, 49) ^ second ^ (second << 21);
    seekRandom[1] = rotate(second, 28);
    if(value <= limit) return 16'934 + value % range;
  }
}

auto Disc::Drive::distance(s32 requested) -> u32 {
  if(seeking || (resetDelay && lba.hold)) updateSeekPosition();
  else updatePosition();
  s32 source = seeking ? lba.seek : resetDelay && lba.hold ? 0 : physical.position;
  u32 current = self.ssr.motorOn ? max(0, source) : 0;
  u32 target = max(0, requested);
  u32 delta = current > target ? current - target : target - current;
  u32 period = 451584 >> mode.speed;
  u32 track = sectorsPerTrack(current);
  u32 jump = current > track ? current - track : 0;
  u32 clocks;
  if(target > current && delta <= track) {
    clocks = period * max(delta, 2u);
  } else if(target <= current && target >= jump) {
    clocks = period * max(target - jump, 1u);
  } else if(delta < 7200) {
    u32 threshold = u32(330.0f - 63.1333f * std::log(std::clamp(float(current) / 4500.0f, 1.0f, 72.0f)));
    clocks = u32((delta < threshold ? 0.05f : 0.1f) * 33'868'800.0f);
  } else {
    constexpr float variable = 0.9f - 0.05f;
    float logarithmic = variable * (std::log(float(delta)) / std::log(324000.0f)) * 0.4f;
    float linear = variable * (float(delta) / 324000.0f) * (1.0f - 0.4f);
    clocks = u32((0.05f + logarithmic + linear) * 33'868'800.0f);
  }
  // The pinned source adds jitter on every call, including non-repeated seeks.
  return clocks + seekJitter();
}

auto Disc::Drive::beginSeek() -> void {
  bool repeated = seekType == SeekType::SeekL && pendingOperation == PendingOperation::None
    && lba.request >= 2 && physical.position == lba.request - 2 && lba.seek == lba.request
    && physical.age < (451584u >> mode.speed);
  u32 delay = repeated ? 30'000
    : distance(lba.request) + (pendingOperation == PendingOperation::Play ? 0 : speedChange) + spinUp;
  if(!repeated && !self.ssr.motorOn && !spinUp) delay += system.frequency();
  headerValid = false;
  lba.start = lba.hold;
  lba.seek = lba.request;
  lba.pending = 0;
  seeking = delay;
  seekInterval = delay;
  seekVisible = true;
  activeStatusCleared = false;
  streamStarting = false;
  self.ssr.reading = self.ssr.playingCDDA = 0;
  speedChange = resetDelay = 0;
  sessionChange = {};
  self.startMotor();
}

auto Disc::Drive::beginStream(PendingOperation operation, bool afterSeek) -> void {
  u8 previousStatus = self.status() & 0xe0;
  u32 delay = afterSeek ? 0 : distance(lba.hold) + spinUp
    + (operation == PendingOperation::Read ? speedChange : 0);
  if(!afterSeek && !self.ssr.motorOn && !spinUp) delay += system.frequency();
  if(operation == PendingOperation::Play) headerValid = false;
  self.counter.sector = -s32(delay);
  lba.current = lba.hold;
  streamStarting = true;
  startingStatus = afterSeek ? (seekVisible && !activeStatusCleared ? 0x40 : 0)
    : operation == PendingOperation::Read ? 0x40 : previousStatus;
  activeStatusCleared = false;
  seeking = 0;
  seekVisible = false;
  seekType = SeekType::None;
  speedChange = resetDelay = 0;
  sessionChange = {};
  pendingOperation = PendingOperation::None;
  if(operation == PendingOperation::Read) lba.start = lba.seek = 0;
  self.ssr.reading = operation == PendingOperation::Read;
  self.ssr.playingCDDA = operation == PendingOperation::Play;
  self.startMotor();
  if(operation == PendingOperation::Play) {
    cdda->playMode = CDDA::PlayMode::Normal;
    cdda->beginReport();
  }
}

auto Disc::Drive::changeSpeed(bool doubleSpeed) -> void {
  bool changed = mode.speed != doubleSpeed;
  s32 oldPeriod = 451584 >> mode.speed;
  mode.speed = doubleSpeed;
  if(!changed) return;
  if(shellOpening) return;  // Reference SetMode changes the latch without delaying automatic shell opening.
  u32 delay = doubleSpeed ? 20'321'280 : 23'708'160;
  if(speedChange) {
    // Reference compares remaining time against a quarter of the NEW transition duration.
    if(speedChange >= delay / 4) speedChange = 0;
  } else if(seeking) {
    seeking += delay;
  } else if(resetDelay) {
    // Reset away from LBA0 is the reference implicit seek: mode changes do not delay it.
    // At LBA0 it owns the same cancellable settling event as an idle speed change.
    if(lba.hold == 0 && resetDelay >= delay / 4) resetDelay = 0;
  } else if(sessionChange.pending) {
    sessionChange.remaining += delay;
  } else if(self.ssr.reading || self.ssr.playingCDDA) {
    // Change the interval after the next sector, preserving its old deadline plus the settling time.
    self.counter.sector += (451584 >> mode.speed) - oldPeriod - s32(delay);
  } else if(spinUp) {
    spinUp += delay;
  } else {
    speedChange = delay;
  }
}

auto Disc::Drive::readSector(s32 position, u8* data) -> bool {
  if(!self.canReadMedia()) return false;
  s64 address = (s64(CD::LeadInSectors) + CD::Track1Pregap + position) * 2448;
  if(address < 0 || u64(address) > self.fd->size() || self.fd->size() - address < 2448) return false;
  self.fd->seek(address);
  return self.fd->readExact({data, 2448});
}

auto Disc::Drive::cacheSubQ(const u8* q) -> bool {
  // H2 firmware notes: adjusted position accepts ADR's low two bits == 1, suppressing UPC/ISRC.
  if((q[0] & 3) != 1 || CD::CRC16({q, 10}) != u16(q[10] << 8 | q[11])) return false;
  for(u32 index : range(10)) sector.subq[index] = q[index];
  return true;
}

auto Disc::Drive::synthesizeLeadOutQ(s32 position) -> bool {
  // Images can omit lead-out. Do not synthesize over missing/invalid program-area Q.
  if(!session->leadOut || position < session->leadOut.lba
    || position - session->leadOut.lba >= CD::LeadOutSectors
    || session->lastTrack >= 100 || !session->tracks[session->lastTrack]) return false;
  auto rel = CD::MSF::fromABA(position - session->leadOut.lba);
  auto abs = CD::MSF::fromLBA(position);
  if(!rel || !abs) return false;
  u8 q[12] = {
    u8(session->tracks[session->lastTrack].control << 4 | 1), 0xaa, 0,
    BCD::encode(rel.minute), BCD::encode(rel.second), BCD::encode(rel.frame), 0,
    BCD::encode(abs.minute), BCD::encode(abs.second), BCD::encode(abs.frame), 0, 0,
  };
  u16 crc = CD::CRC16({q, 10});
  q[10] = crc >> 8;
  q[11] = crc;
  return cacheSubQ(q);
}

auto Disc::Drive::sampleSubQ(s32 position) -> bool {
  if(!self.canReadMedia()) return false;
  s64 address = (s64(CD::LeadInSectors) + CD::Track1Pregap + position) * 2448 + 2352 + 12;
  if(address < 0) return false;
  if(u64(address) > self.fd->size() || self.fd->size() - address < 12) {
    return synthesizeLeadOutQ(position);
  }
  u8 q[12];
  self.fd->seek(address);
  return self.fd->readExact({q, 12}) && cacheSubQ(q);
}

auto Disc::Drive::updateSubQ() -> bool {
  if(!physical.pending) return true;
  physical.pending = false;
  return sampleSubQ(physical.position);
}

auto Disc::Drive::setHold(s32 position, s32 subq) -> void {
  physical.pending |= physical.position != subq;
  lba.current = lba.hold = position;
  physical.position = subq;
  physical.age = physical.carry = 0;
}

auto Disc::Drive::updateSeekPosition() -> void {
  if(!seekInterval) return;
  u32 remaining = seeking ? seeking : resetDelay;
  s32 target = seeking ? lba.seek : 0;
  if(target == lba.start) return;
  float fraction = 1.0f - min(float(remaining) / float(seekInterval), 1.0f);
  s32 delta = target > lba.start ? target - lba.start : lba.start - target;
  s32 movement = max(s32(float(delta) * fraction), 1);
  s32 position = lba.start + (target > lba.start ? movement : -movement);
  physical.pending = physical.position != position;
  physical.position = position;
  physical.age = physical.carry = 0;
}

auto Disc::Drive::updatePosition(bool logical) -> void {
  if(seeking || (resetDelay && lba.hold) || self.ssr.reading || self.ssr.playingCDDA || !self.ssr.motorOn) {
    if((self.status() & 0xa2) == 2 && physical.position != lba.hold) setHold(lba.hold, lba.hold);
    return;
  }
  u32 period = 451584 >> mode.speed;
  u64 elapsed = physical.age + physical.carry;
  u32 sectors = elapsed / period;
  if(!sectors) return;
  u32 track = sectorsPerTrack(lba.hold);
  s32 end = lba.hold + (headerValid ? 2 : 0);
  u32 start = max(0, end - s32(track));
  // The reference offset arithmetic is unsigned, including before the hold window's start.
  u32 offset = u32(physical.position) - start;
  s32 position = start + (offset + sectors) % track;
  if(physical.position == position) return;
  physical.position = position;
  physical.pending = true;
  physical.age = 0;
  physical.carry = elapsed % period;
  if(logical) {
    u8 data[2448];
    if(readSector(position, data)) {
      physical.pending = false;
      cacheSubQ(data + 2352 + 12);
      cacheHeader(data);
    }
  }
}

auto Disc::Drive::finishSeek() -> void {
  if(!self.canReadMedia()) return;
  u8 data[2448];
  bool success = readSector(lba.seek, data);
  physical.position = lba.seek;
  physical.age = physical.carry = 0;
  physical.pending = false;
  if(success) {
    lba.current = lba.hold = lba.seek;
    const u8* q = data + 2352 + 12;
    // Reference fallback: invalid CRC bypasses position/type validation, preserving the last good Q.
    cacheSubQ(q);
    if(CD::CRC16({q, 10}) == u16(q[10] << 8 | q[11])) {
      auto position = CD::MSF::fromABA(CD::LBAtoABA(lba.current));
      u8 minute = BCD::encode(position.minute), second = BCD::encode(position.second);
      u8 frame = BCD::encode(position.frame);
      success = q[7] == minute && q[8] == second && q[9] == frame;
      if(success && seekType == SeekType::SeekL) {
        if(q[0] & 0x40) {
          cacheHeader(data);
          success = data[12] == minute && data[13] == second && data[14] == frame;
          if(success && pendingOperation == PendingOperation::None) {
            physical.position = max(0, lba.hold - 2);
            physical.pending = true;
          }
        } else if(pendingOperation == PendingOperation::Read) {
          success = mode.cdda;
        }
      }
      if(q[1] == 0xaa) success = false;
    }
  }
  self.counter.sector = 0;
  if(!success) {
    self.stopDriveActivity();
    headerValid = false;
    self.queueResponse(ResponseType::Error, {u8(self.status() | 4), ErrorCode_SeekFailed}, true);
  } else if(pendingOperation != PendingOperation::None) {
    beginStream(pendingOperation, true);
  } else {
    self.ssr.reading = self.ssr.playingCDDA = 0;
    activeStatusCleared = false;
    self.queueResponse(ResponseType::Complete, {self.status()});
  }
  pendingOperation = PendingOperation::None;
  seekType = SeekType::None;
  seekVisible = false;
  lba.pending = 0;  // Pinned reference clears a later Setloc latch when an explicit seek completes.
}

auto Disc::Drive::clockSector() -> void {
  if(!likely(self.fd)) return;
  if(!self.canReadMedia() || sessionChange.pending || spinUp || resetDelay || !self.ssr.motorOn) return;
  if(seeking || speedChange) return;

  if(!seeking && (self.ssr.reading || self.ssr.playingCDDA) && lba.current >= session->leadOut.lba) {
    streamStarting = activeStatusCleared = false;
    return finishReading(true);
  }

  if(self.ssr.reading || self.ssr.playingCDDA) {
    u8 data[2448];
    if(!readSector(lba.current, data)) {
      self.fifo.deferred = {};
      self.queueResponse(ResponseType::Error, {u8(self.status() | 1), ErrorCode_CannotRespondYet}, true);
      self.stopDriveActivity();
      return;
    }
    streamStarting = activeStatusCleared = false;
    self.debugger.read(lba.current);
    for(u32 index : range(2448)) sector.data[index] = data[index];
    // MiSTer PR332: mech Q leads decoder data by two sectors during streaming.
    // Read the Q source independently so native Q and SBI/LSD CRC/ADR survive.
    // This models a position relationship, not an additional data delivery delay.
    sampleSubQ(lba.current + 2);
    const u8* rawQ = data + 2352 + 12;
    bool validSubQ = (rawQ[0] & 3) == 1 && CD::CRC16({rawQ, 10}) == u16(rawQ[10] << 8 | rawQ[11]);
    lba.hold = physical.position = lba.current;
    physical.age = physical.carry = 0;
    physical.pending = false;
    lba.current++;

    if(auto trackID = session->inTrack(lba.current - 1)) {
      sector.track = *trackID;
    } else {
      sector.track = 0;
    }

    const auto* q = sector.data + 2352 + 12;
    if(q[1] == 0xaa) return finishReading(true);  // Reference lead-out marker, including raw invalid-CRC Q.
    bool isData = q[0] & 0x40;  // Current raw SubQ control, as in the reference.
    if(isData) cacheHeader(sector.data);
    else {
      headerValid = false;
      if(mode.autoPause) {
        if(cdda->autoPausePending) return finishReading(false);
        if(!cdda->playTrack) cdda->playTrack = q[1];
        if(cdda->playTrack != q[1]) cdda->autoPausePending = true;
        else cdda->holdLBA = lba.current - 1;
      }
    }
    if(self.ssr.playingCDDA) cdda->started = true;
    if(!isData && (self.ssr.playingCDDA || mode.cdda)) {
      if(cdda->scanStep) {
        lba.current = lba.current - 1 + cdda->scanStep;
        if(lba.current <= 0) {
          lba.current = 0;
          cdda->scanStep = 0;
          cdda->playMode = CDDA::PlayMode::Normal;
        }
      }
      return cdda->clockSector(validSubQ);
    }
    if(!isData || !self.ssr.reading) return;

    if(sector.data[15] == 0x02) {
      if(mode.xaADPCM && (sector.data[18] & 0x44) == 0x44) {
        return cdxa->clockSector();
      }
      if(mode.xaFilter && (sector.data[18] & 0x44) == 0x44) {
        return;
      }
    }

    self.receiveSector(sector.data);
  }
}

auto Disc::Drive::finishReading(bool stopMotor) -> void {
  // H2 end event; reference response snapshots active status before stopping.
  self.fifo.deferred = {};
  self.queueResponse(ResponseType::End, {self.status()});
  self.stopDriveActivity();
  cdda->autoPausePending = false;
  if(stopMotor) {
    self.ssr.motorOn = 0;
    headerValid = false;
    setHold(0, 0);
  } else {
    // H2 normal playback holds the old track end. Scan/AutoPause positioning is unknown: use reference position.
    s32 position = cdda->scanStep ? lba.hold : cdda->holdLBA;
    setHold(position, position);
    updateSubQ();
  }
}

auto Disc::Drive::cacheHeader(const u8* data) -> void {
  for(u32 index : range(8)) header[index] = data[12 + index];
  headerValid = true;
}

auto Disc::Drive::sectorsPerTrack(s32 position) const -> u32 {
  // Reference mech table indexed by elapsed minute (DISC-014, R2).
  static constexpr u8 lastMinute[] = {0, 4, 7, 11, 16, 23, 27, 32, 39, 44, 52, 60, 67, 74};
  u32 minute = max(0, position) / (75 * 60);
  for(u32 index : range(14)) if(minute <= lastMinute[index]) return 8 + index;
  return 22;
}

auto Disc::Drive::pauseDelay() const -> u32 {
  if(!self.ssr.reading && !self.ssr.playingCDDA) return 7400;
  u32 sectors = sectorsPerTrack(lba.hold) - (self.ssr.reading && !self.ssr.playingCDDA ? 2 : 0);
  s32 elapsed = self.counter.sector;
  s32 delay = sectors * (451584 >> mode.speed) - elapsed;
  return max(delay, mode.speed ? 1'000'000 : 2'000'000);
}

auto Disc::receiveSector(const u8* sector) -> void {
  auto mode = sector[15];
  if(!drive.mode.sectorSize && mode != 1 && mode != 2) return;

  // A selected slot can wrap before BFRD. Once requested, its host snapshot remains intact.
  auto& buffer = fifo.sectors[++fifo.sectorWrite];
  buffer.flush();
  if(!drive.mode.sectorSize) {
    u32 start = mode == 1 ? 16 : 24;
    for(u32 offset : range(2048)) buffer.write(sector[start + offset]);
  } else if(mode == 1) {
    // Reference-derived Mode1 raw transfer: synthesize the decoder's Mode2-shaped subheader.
    for(u32 offset : range(4)) buffer.write(sector[12 + offset]);
    for(u32 offset : range(8)) buffer.write(0);
    for(u32 offset : range(2048)) buffer.write(sector[16 + offset]);
  } else {
    for(u32 offset : range(2340)) buffer.write(sector[12 + offset]);
  }
  fifo.sectorPending = true;
  fifo.sectorFile = sector[16];
  fifo.sectorChannel = sector[17];
  fifo.sectorNeedsFilter = mode == 2 && irq.pending();
  queueResponse(ResponseType::Ready, {status()});
}
