#include "mega-cd.cpp"
#include "pc-engine-cd.cpp"
#include "playstation.cpp"

auto CompactDisc::construct() -> void {
  Media::construct();
}

//audio CD fallback
auto CompactDisc::manifest(string location) -> string {
  string manifest;
  manifest += "game\n";
  manifest +={"  name:  ", Location::prefix(location), "\n"};
  manifest +={"  title: ", Location::prefix(location), "\n"};
  manifest += "  audio\n";
  return manifest;
}

auto CompactDisc::readDataSectorBCD(string pathname, u32 sectorID) -> vector<u8> {
  auto fp = file::open({pathname, "cd.rom"}, file::mode::read);
  if(!fp) return {};

  vector<u8> toc;
  toc.resize(96 * 7500);
  for(u32 sector : range(7500)) {
    fp.read({toc.data() + 96 * sector, 96});
  }
  CD::Session session;
  session.decode(toc, 96);

  for(u32 trackID : range(100)) {
    if(auto& track = session.tracks[trackID]) {
      if(!track.isData()) continue;
      if(auto index = track.index(1)) {
        vector<u8> sector;
        sector.resize(2448);
        fp.seek(2448 * (abs(session.leadIn.lba) + index->lba + sectorID) + 16);
        fp.read({sector.data(), 2448});
        return sector;
      }
    }
  }

  return {};
}

auto CompactDisc::readDataSectorCUE(string filename, u32 sectorID) -> vector<u8> {
  Decode::CUE cuesheet;
  if(!cuesheet.load(filename)) return {};

  for(auto& file : cuesheet.files) {
    u64 offset = 0;
    auto location = string{Location::path(filename), file.name};

    if(file.type == "binary") {
      auto binary = file::open(location, nall::file::mode::read);
      if(!binary) continue;
      for(auto& track : file.tracks) {
        for(auto& index : track.indices) {
          u32 sectorSize = 0;
          if(track.type == "mode1/2048") sectorSize = 2048;
          if(track.type == "mode1/2352") sectorSize = 2352;
          if(track.type == "mode2/2352") sectorSize = 2352;
          if(sectorSize && index.number == 1) {
            binary.seek(offset + (sectorSize * sectorID) + (sectorSize == 2352 ? 16 : 0));
            vector<u8> sector;
            sector.resize(2048);
            binary.read({sector.data(), sector.size()});
            return sector;
          }
          offset += track.sectorSize() * index.sectorCount();
        }
      }
    }

    if(file.type == "wave") {
      Decode::WAV wave;
      if(!wave.open(location)) continue;
      offset += wave.headerSize;
      for(auto& track : file.tracks) {
        auto length = track.sectorSize();
        for(auto& index : track.indices) {
          offset += track.sectorSize() * index.sectorCount();
        }
      }
    }
  }

  return {};
}
