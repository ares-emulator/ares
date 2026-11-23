#include <ares/ares.hpp>
#include "cdrom.hpp"

namespace ares {

#include "serialization.cpp"

auto SCSICDROM::power() -> void {
  SCSIDevice::power();
  buffer.clear();
  position = 0;
  readRemaining = 0;
  sectorPosition = sectorLength = 0;
  outRemaining = 0;
  outBuffer.clear();
  senseKey = senseASC = senseASCQ = 0;
  blockSize = 2048;
  audioStatus = 0x15;
  audioPosition = audioEnd = 0;
}

auto SCSICDROM::ready() -> bool {
  return session && readSector && session() != nullptr;
}

auto SCSICDROM::checkCondition(n8 key, n8 asc, n8 ascq) -> void {
  status = 2;
  senseKey = key;
  senseASC = asc;
  senseASCQ = ascq;
  buffer.clear();
  position = 0;
  readRemaining = 0;
  sectorPosition = sectorLength = 0;
}

auto SCSICDROM::respond(std::vector<u8> data, u32 limit) -> void {
  if(data.size() > limit) data.resize(limit);
  buffer = std::move(data);
  position = 0;
}

auto SCSICDROM::dataInAvailable() -> bool {
  return position < buffer.size() || sectorPosition < sectorLength || readRemaining > 0;
}

auto SCSICDROM::dataIn() -> n8 {
  if(position < buffer.size()) return buffer[position++];
  if(sectorPosition >= sectorLength && readRemaining) {
    if(!loadSector()) return 0;
  }
  if(sectorPosition < sectorLength) return raw[sectorOffset + sectorPosition++];
  return 0;
}

auto SCSICDROM::loadSector() -> bool {
  if(!readRemaining) return false;
  readRemaining--;
  sectorPosition = sectorLength = 0;
  if(!readSector || !readSector(readLBA, raw)) {
    readRemaining = 0;
    return false;
  }
  readLBA++;
  bool audio = false;
  if(auto* current = session ? session() : nullptr) {
    if(auto track = current->inTrack(readLBA - 1)) audio = current->tracks[*track].isAudio();
  }
  if(audio) {
    sectorOffset = 0;
    sectorLength = 2352;
  } else if(blockSize == 2336) {
    sectorOffset = 16;
    sectorLength = 2336;
  } else if(blockSize == 2340) {
    sectorOffset = 12;
    sectorLength = 2340;
  } else if(blockSize == 2352) {
    sectorOffset = 0;
    sectorLength = 2352;
  } else {
    sectorOffset = raw[15] == 2 ? 24 : 16;
    sectorLength = 2048;
  }
  return true;
}

auto SCSICDROM::readBlocks(s32 lba, u32 count) -> void {
  auto* current = session ? session() : nullptr;
  if(!current) return checkCondition(2, 0x3a);
  if(lba < 0 || lba >= current->leadOut.lba) return checkCondition(5, 0x21);
  if(lba + (s32)count > current->leadOut.lba) return checkCondition(5, 0x21);
  buffer.clear();
  position = 0;
  readLBA = lba;
  readRemaining = count;
  sectorPosition = sectorLength = 0;
  status = 0;
}

auto SCSICDROM::readToc(std::span<const u8> cdb) -> void {
  auto* current = session ? session() : nullptr;
  if(!current) return checkCondition(2, 0x3a);
  bool msf = cdb[1] & 2;
  u32 format = cdb[2] & 15;
  if(format != 0) return checkCondition(5, 0x24);
  u32 start = cdb[6];
  u32 limit = cdb[7] << 8 | cdb[8];

  std::vector<u8> out(4);
  auto descriptor = [&](u32 number, n8 control, s32 lba) {
    out.push_back(0);
    out.push_back(0x10 | control);
    out.push_back(number);
    out.push_back(0);
    if(msf) {
      auto time = CD::MSF::fromLBA(lba);
      out.push_back(0);
      out.push_back(time.minute);
      out.push_back(time.second);
      out.push_back(time.frame);
    } else {
      out.push_back(lba >> 24);
      out.push_back(lba >> 16);
      out.push_back(lba >> 8);
      out.push_back(lba);
    }
  };

  u32 first = 99, last = 0;
  for(u32 number : range(1, 100)) {
    if(!current->tracks[number]) continue;
    first = min(first, number);
    last = max(last, number);
  }
  if(last == 0) return checkCondition(5, 0x24);
  if(start == 0) start = first;
  if(start != 0xaa && (start < first || start > last)) return checkCondition(5, 0x24);
  for(u32 number : range(start, last + 1)) {
    if(!current->tracks[number]) continue;
    descriptor(number, current->tracks[number].control, current->tracks[number].indices[1].lba);
  }
  descriptor(0xaa, 0x04, current->leadOut.lba);

  u32 length = out.size() - 2;
  out[0] = length >> 8;
  out[1] = length;
  out[2] = first;
  out[3] = last;
  respond(std::move(out), limit ? limit : 0xffff);
  status = 0;
}

auto SCSICDROM::setIdentity(string vendor, string product, string revision) -> void {
  identity = {vendor, product, revision};
}

auto SCSICDROM::playAudio(s32 start, s32 end) -> void {
  auto* current = session ? session() : nullptr;
  if(!current) return checkCondition(2, 0x3a);
  if(start < 0 || start >= current->leadOut.lba) return checkCondition(5, 0x21);
  audioPosition = start;
  audioEnd = min(end, current->leadOut.lba);
  audioStatus = audioPosition < audioEnd ? 0x11 : 0x13;
  status = 0;
}

auto SCSICDROM::audioTick() -> void {
  if(audioStatus != 0x11) return;
  if(++audioPosition >= audioEnd) audioStatus = 0x13;
}

auto SCSICDROM::readSubchannel(std::span<const u8> cdb) -> void {
  if(!ready()) return checkCondition(2, 0x3a);
  bool msf = cdb[1] & 2;
  bool subQ = cdb[2] & 0x40;
  u32 format = cdb[3];
  u32 limit = cdb[7] << 8 | cdb[8];
  std::vector<u8> out = {0, audioStatus, 0, 0};
  if(subQ) {
    if(format != 1) return checkCondition(5, 0x24);
    out[3] = 12;
    out.push_back(1);
    out.push_back(0x14);
    out.push_back(1);
    out.push_back(1);
    auto* current = session();
    s32 trackStart = 0;
    u32 trackNumber = 1;
    if(auto track = current->inTrack(audioPosition)) {
      trackNumber = *track;
      trackStart = current->tracks[*track].indices[1].lba;
    }
    out[6] = trackNumber;
    out[5] = 0x10 | (current->tracks[trackNumber] ? current->tracks[trackNumber].control : 0x04);
    s32 values[2] = {audioPosition, audioPosition - trackStart};
    for(s32 value : values) {
      if(msf) {
        auto time = CD::MSF::fromLBA(value);
        out.push_back(0);
        out.push_back(time.minute);
        out.push_back(time.second);
        out.push_back(time.frame);
      } else {
        out.push_back(value >> 24);
        out.push_back(value >> 16);
        out.push_back(value >> 8);
        out.push_back(value);
      }
    }
  }
  respond(std::move(out), limit);
  status = 0;
}

auto SCSICDROM::modeSense(std::span<const u8> cdb) -> void {
  u32 page = cdb[2] & 0x3f;
  bool descriptor = !(cdb[1] & 8);
  std::vector<u8> out = {0, 0, 0, (n8)(descriptor ? 8 : 0)};
  if(descriptor) {
    for(u32 byte : range(5)) out.push_back(0);
    out.push_back(blockSize >> 16);
    out.push_back(blockSize >> 8);
    out.push_back(blockSize);
  }
  if(page == 0x0d || page == 0x3f) {
    std::vector<u8> data = {0x0d, 0x06, 0, 0x0d, 0, 60, 0, 75};
    out.insert(out.end(), data.begin(), data.end());
  }
  if(page == 0x0e || page == 0x3f) {
    std::vector<u8> data = {0x0e, 0x0e, 0x04, 0, 0, 0, 0, 0, 0x01, 0xff, 0x02, 0xff, 0, 0, 0, 0};
    out.insert(out.end(), data.begin(), data.end());
  }
  out[0] = out.size() - 1;
  respond(std::move(out), cdb[4]);
  status = 0;
}

auto SCSICDROM::execute(std::span<const u8> cdb) -> void {
  status = 0;
  buffer.clear();
  position = 0;
  readRemaining = 0;
  sectorPosition = sectorLength = 0;
  if(cdb.empty()) return;

  n8 opcode = cdb[0];
  std::vector<u8> cdbCopy(cdb.begin(), cdb.end());
  cdbCopy.resize(12, 0);
  cdb = cdbCopy;

  switch(opcode) {
  case 0x00:
    if(!ready()) return checkCondition(2, 0x3a);
    return;

  case 0x03: {
    std::vector<u8> sense(18, 0);
    sense[0] = 0x70;
    sense[2] = senseKey;
    sense[7] = 10;
    sense[12] = senseASC;
    sense[13] = senseASCQ;
    senseKey = senseASC = senseASCQ = 0;
    respond(std::move(sense), cdb[4] ? cdb[4] : 4);
    return;
  }

  case 0x12: {
    std::vector<u8> data(36, 0);
    data[0] = 0x05;
    data[1] = 0x80;
    data[2] = 0x02;
    data[3] = 0x02;
    data[4] = 31;
    auto fill = [&](u32 offset, u32 size, const string& text) {
      for(u32 n : range(size)) data[offset + n] = n < text.size() ? text.data()[n] : ' ';
    };
    fill(8, 8, identity.vendor);
    fill(16, 16, identity.product);
    fill(32, 4, identity.revision);
    respond(std::move(data), cdb[4]);
    return;
  }

  case 0x15:
    outRemaining = cdb[4];
    outBuffer.clear();
    outModeSelect = true;
    return;

  case 0x1a:
    return modeSense(cdb);

  case 0x1b:
  case 0x1e:
  case 0x2b:
    if(!ready()) return checkCondition(2, 0x3a);
    return;

  case 0x25: {
    auto* current = session ? session() : nullptr;
    if(!current) return checkCondition(2, 0x3a);
    s32 last = current->leadOut.lba - 1;
    std::vector<u8> data = {(n8)(last >> 24), (n8)(last >> 16), (n8)(last >> 8), (n8)last, 0, 0, 8, 0};
    respond(std::move(data), 8);
    return;
  }

  case 0x08: {
    s32 lba = (cdb[1] & 0x1f) << 16 | cdb[2] << 8 | cdb[3];
    u32 count = cdb[4] ? cdb[4] : 256;
    return readBlocks(lba, count);
  }

  case 0x28: {
    s32 lba = cdb[2] << 24 | cdb[3] << 16 | cdb[4] << 8 | cdb[5];
    u32 count = cdb[7] << 8 | cdb[8];
    if(!count) return;
    return readBlocks(lba, count);
  }

  case 0x43:
    return readToc(cdb);

  case 0x42:
    return readSubchannel(cdb);

  case 0x45: {
    s32 start = cdb[2] << 24 | cdb[3] << 16 | cdb[4] << 8 | cdb[5];
    s32 length = cdb[7] << 8 | cdb[8];
    return playAudio(start, start + length);
  }

  case 0x47: {
    s32 start = CD::MSF(cdb[3], cdb[4], cdb[5]).toLBA();
    s32 end = CD::MSF(cdb[6], cdb[7], cdb[8]).toLBA();
    return playAudio(start, end);
  }

  case 0x48: {
    auto* current = session ? session() : nullptr;
    if(!current) return checkCondition(2, 0x3a);
    u32 first = cdb[4], last = cdb[7];
    if(first < 1 || first > 99 || last < first || last > 99 || !current->tracks[first]) return checkCondition(5, 0x24);
    s32 end = current->leadOut.lba;
    for(u32 number = last; number <= 99; number++) {
      if(number == last) continue;
      if(current->tracks[number]) { end = current->tracks[number].indices[1].lba; break; }
    }
    return playAudio(current->tracks[first].indices[1].lba, end);
  }

  case 0x4b:
    if(!ready()) return checkCondition(2, 0x3a);
    if(cdb[8] & 1) { if(audioStatus == 0x12) audioStatus = 0x11; }
    else if(audioStatus == 0x11) audioStatus = 0x12;
    return;

  case 0x4e:
    if(!ready()) return checkCondition(2, 0x3a);
    audioStatus = 0x13;
    return;
  }

  return checkCondition(5, 0x20);
}

auto SCSICDROM::dataOutExpected() -> bool {
  return outRemaining > 0;
}

auto SCSICDROM::dataOut(n8 data) -> void {
  if(!outRemaining) return;
  outBuffer.push_back(data);
  if(--outRemaining) return;
  if(outModeSelect && outBuffer.size() >= 12) {
    u32 length = outBuffer[9] << 16 | outBuffer[10] << 8 | outBuffer[11];
    if(length == 2048 || length == 2336 || length == 2340 || length == 2352) blockSize = length;
  }
  outModeSelect = false;
}


}
