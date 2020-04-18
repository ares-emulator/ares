#pragma once

#include <nall/array-span.hpp>
#include <nall/cd.hpp>
#include <nall/file.hpp>
#include <nall/string.hpp>
#include <nall/decode/cue.hpp>
#include <nall/decode/wav.hpp>

namespace nall::vfs {

struct cdrom : file {
  static auto open(const string& cueLocation) -> shared_pointer<cdrom> {
    auto instance = shared_pointer<cdrom>{new cdrom};
    if(instance->load(cueLocation)) return instance;
    return {};
  }

  auto size() const -> uintmax override {
    return _image.size();
  }

  auto offset() const -> uintmax override {
    return _offset;
  }

  auto seek(intmax offset, index mode) -> void override {
    if(mode == index::absolute) _offset = (uintmax)offset;
    if(mode == index::relative) _offset += (intmax)offset;
  }

  auto read() -> uint8_t override {
    if(_offset >= _image.size()) return 0x00;
    return _image[_offset++];
  }

  auto write(uint8_t data) -> void override {
    //CD-ROMs are read-only; but allow writing anyway if needed, since the image is in memory
    if(_offset >= _image.size()) return;
    _image[_offset++] = data;
  }

private:
  auto load(const string& cueLocation) -> bool {
    Decode::CUE cuesheet;
    if(!cuesheet.load(cueLocation)) return false;

    CD::Session session;
    session.leadIn.lba = -LeadInSectors;
    session.leadIn.end = -1;
    int lbaDisc = Track1Pregap;
    int endDisc = lbaDisc;
    for(auto& file : cuesheet.files) {
      for(auto& track : file.tracks) {
        session.tracks[track.number].control = track.type == "audio" ? 0b0000 : 0b0100;
        session.tracks[track.number].address = 0b0001;
        for(auto& index : track.indices) {
          session.tracks[track.number].indices[index.number].lba = lbaDisc + index.lba;
          session.tracks[track.number].indices[index.number].end = lbaDisc + index.end;
        }
      }
      lbaDisc += file.tracks.last().indices.last().end + 1;
      endDisc = lbaDisc;
    }
    session.leadOut.lba = endDisc;
    session.leadOut.end = endDisc + LeadOutSectors - 1;

    for(uint track : range(100)) {
      if(!session.tracks[track]) continue;
      session.firstTrack = track;
      for(uint index : range(100)) {
        if(!session.tracks[track].indices[index]) continue;
        session.tracks[track].firstIndex = index;
        break;
      }
      break;
    }

    for(uint track : reverse(range(100))) {
      if(!session.tracks[track]) continue;
      session.lastTrack = track;
      for(uint index : reverse(range(100))) {
        if(!session.tracks[track].indices[index]) continue;
        session.tracks[track].lastIndex = index;
        break;
      }
      break;
    }

    session.tracks[1].indices[0].lba = 0;  //track 1, index 0 is not present in CUE files
    session.tracks[1].indices[0].end = Track1Pregap - 1;

    _image.resize(2448 * (LeadInSectors + endDisc + LeadOutSectors));

    lbaDisc = Track1Pregap;
    for(auto& file : cuesheet.files) {
      auto location = string{Location::path(cueLocation), file.name};
      auto filedata = nall::file::open(location, nall::file::mode::read);
      if(file.type == "wave") filedata.seek(44);  //skip RIFF header
      uint64_t offset = 0;
      for(auto& track : file.tracks) {
        for(auto& index : track.indices) {
          for(int sector : range(index.sectorCount())) {
            auto target = _image.data() + 2448ull * (LeadInSectors + lbaDisc + index.lba + sector);
            auto length = track.sectorSize();
            if(length == 2048) {
              //ISO: generate header + parity data
              memory::assign(target + 0, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff);  //sync
              memory::assign(target + 6, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00);  //sync
              auto [minute, second, frame] = CD::MSF(lbaDisc + index.lba + sector);
              target[12] = CD::BCD::encode(minute);
              target[13] = CD::BCD::encode(second);
              target[14] = CD::BCD::encode(frame);
              target[15] = 0x01;  //mode
              filedata.read({target + 16, length});
              CD::RSPC::encodeMode1({target, 2352});
            }
            if(length == 2352) {
              //BIN + WAV: direct copy
              filedata.read({target, length});
            }
          }
        }
        offset += track.sectorSize() * track.sectorCount();
      }
      lbaDisc += file.tracks.last().indices.last().end + 1;
    }

    auto subchannel = session.encode(LeadInSectors + session.leadOut.end + 1);
    if(auto overlay = nall::file::read({Location::notsuffix(cueLocation), ".sub"})) {
      auto target = subchannel.data() + 96 * (LeadInSectors + Track1Pregap);
      auto length = (int64_t)subchannel.size() - 96 * (LeadInSectors + Track1Pregap);
      memory::copy(target, length, overlay.data(), overlay.size());
    }

    for(uint64_t sector : range(size() / 2448)) {
      auto source = subchannel.data() + sector * 96;
      auto target = _image.data() + sector * 2448 + 2352;
      memory::copy(target, source, 96);
    }

    return true;
  }

  vector<uint8_t> _image;
  uintmax _offset = 0;

  static constexpr int LeadInSectors  = 7500;
  static constexpr int Track1Pregap   =  150;
  static constexpr int LeadOutSectors = 6750;
};

}
