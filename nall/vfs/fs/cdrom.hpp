#pragma once

#include <nall/array-view.hpp>
#include <nall/cd.hpp>
#include <nall/file.hpp>
#include <nall/string.hpp>
#include <nall/decode/cue.hpp>

namespace nall::vfs::fs {

struct cdrom : vfs::file {
  static auto open(const string& cueLocation) -> shared_pointer<vfs::file> {
    Decode::CUE cue;
    cue.load(string::read(cueLocation));
    if(auto seek = cue.seek(1)) {
      string imgLocation = {Location::dir(cueLocation), seek->name};
      auto instance = shared_pointer<cdrom>{new cdrom};
      if(seek->type == "mode1/2352") {
        if(!instance->binParse(imgLocation)) return {};
      } else if(seek->type == "mode1/2048") {
        if(!instance->isoParse(imgLocation)) return {};
      } else {
        return {};
      }
      if(!instance->cueParse(cueLocation)) return {};
      string subLocation = {Location::notsuffix(cueLocation), ".sub"};
      instance->subParse(subLocation);  //optional (can be auto-generated via CUE file)
      return instance;
    }
    return {};
  }

  auto size() const -> uintmax override {
    uint64_t sectors = abs(session.leadIn.lba) + 150 + imageSectorCount + 6750;
    return sectors * 2448;
  }

  auto offset() const -> uintmax override {
    return sectorNumber * 2448 + sectorOffset;
  }

  auto seek(intmax offset, index mode) -> void override {
    uint64_t address = sectorNumber * 2448 + sectorOffset;
    if(mode == index::absolute) address = (uintmax)offset;
    if(mode == index::relative) address += (intmax)offset;

    sectorNumber = address / 2448;
    sectorOffset = address % 2448;
    sectorReady = false;
  }

  auto read() -> uint8_t override {
    if(!sectorReady) loadSector(), sectorReady = true;
    uint8_t data = sectorBuffer[sectorOffset];
    if(++sectorOffset >= 2448) sectorNumber++, sectorOffset = 0, sectorReady = false;
    return data;
  }

  auto write(uint8_t data) -> void override {
    //read-only medium
  }

private:
  auto loadSector() -> void {
    if(sectorNumber <  abs(session.leadIn.lba) + 150
    || sectorNumber >= abs(session.leadIn.lba) + 150 + imageSectorCount
    ) {
      //lead-in, track 1 pregap, and lead-out is not present in image files.
      memory::fill(&sectorBuffer[0], 2352);
    } else if(imageSectorSize == 2352) {
      image.seek((sectorNumber - abs(session.leadIn.lba) - 150) * 2352);
      image.read({&sectorBuffer[0], 2352});
    } else if(imageSectorSize == 2048) {
      image.seek((sectorNumber - abs(session.leadIn.lba) - 150) * 2048);
      image.read({&sectorBuffer[0], 2048});
      CD::RSPC::encodeMode1({&sectorBuffer[0], 2352});  //generate missing parity data
    }
    if(sectorNumber * 96 + 95 >= subchannel.size()) {
      //out-of-range (past lead-out area of disc.)
      memory::fill(&sectorBuffer[2352], 96);
    } else {
      memory::copy(&sectorBuffer[2352], &subchannel[sectorNumber * 96], 96);
    }
  }

  auto binParse(const string& binLocation) -> bool {
    if(!image.open(binLocation, nall::file::mode::read)) return false;
    if(image.size() % 2352) return false;
    imageSectorSize = 2352;
    imageSectorCount = image.size() / imageSectorSize;
    return true;
  }

  auto isoParse(const string& isoLocation) -> bool {
    if(!image.open(isoLocation, nall::file::mode::read)) return false;
    if(image.size() % 2048) return false;
    imageSectorSize = 2048;
    imageSectorCount = image.size() / imageSectorSize;
    return true;
  }

  auto cueParse(const string& cueLocation) -> bool {
    session = {};
    session.leadIn.lba = -7500;
    session.leadOut.lba = 150 + imageSectorCount;

    int track = -1;
    auto lines = string::read(cueLocation).replace("\r", "").split("\n").strip();

    for(auto& line : lines) {
      if(line.beginsWith("TRACK ")) {
        line.trimLeft("TRACK ", 1L).strip();
        int control = 0b1111;
        int address = 0b0001;
        if(line.endsWith(" AUDIO")) {
          line.trimRight(" AUDIO", 1L).strip();
          control = 0b0000;
        } else if(line.endsWith(" MODE1/2352")) {
          line.trimRight(" MODE1/2352", 1L).strip();
          control = 0b0100;
        } else if(line.endsWith(" MODE1/2048")) {
          line.trimRight(" MODE1/2048", 1L).strip();
          control = 0b0100;
        } else {
          return false;
        }

        track = line.natural();
        if(track > 99) return false;
        session.tracks[track].control = control;
        session.tracks[track].address = address;
        continue;
      }

      if(line.beginsWith("INDEX ")) {
        if(track == -1) return false;
        line.trimLeft("INDEX ", 1L).strip();
        auto part = line.split(" ", 1L);
        if(part.size() != 2) return false;
        int index = part[0].natural();
        if(index > 99) return false;
        auto time = part[1].split(":");
        if(time.size() != 3) return false;
        auto msf = CD::MSF(time[0].natural(), time[1].natural(), time[2].natural());
        if(!msf) return false;
        msf = CD::MSF::fromLBA(msf.toLBA() + 150);
        if(!msf) return false;
        session.tracks[track].indices[index].lba = msf.toLBA();
        continue;
      }
    }

    for(uint track : range(100)) {
      if(!session.tracks[track]) continue;
      session.firstTrack = track;
      break;
    }

    for(uint track : reverse(range(100))) {
      if(!session.tracks[track]) continue;
      session.lastTrack = track;
      break;
    }

    session.tracks[1].indices[0].lba = 0;  //track 1, index 0 is not present in CUE files.
    session.synchronize(6750);
    subchannel = session.encode(abs(session.leadIn.lba) + 150 + imageSectorCount + 6750);
    return true;
  }

  //overlay an existing subchannel data file, if it exists.
  //SUB files are missing lead-in, track 1 pregap, and lead-out.
  auto subParse(const string& subLocation) -> bool {
    auto overlay = nall::file::read(subLocation);
    if(!overlay) return false;
    if(overlay.size() != image.size() * 96 / imageSectorSize) return false;

    uint address = (abs(session.leadIn.lba) + 150) * 96;
    memory::copy(subchannel.data() + address, subchannel.size() - address, overlay.data(), overlay.size());
    return true;
  }

private:
  file_buffer image;
  CD::Session session;
  vector<uint8_t> subchannel;

  bool sectorReady = false;
  uint sectorNumber = 0;
  uint sectorOffset = 0;
  uint8_t sectorBuffer[2448] = {};

  uint imageSectorSize = 0;
  uint imageSectorCount = 0;
};

}
