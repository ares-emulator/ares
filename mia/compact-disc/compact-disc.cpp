#include "mega-cd.cpp"
#include "pc-engine-cd.cpp"
#include "playstation.cpp"

auto CompactDisc::construct() -> void {
  Media::construct();
}

auto CompactDisc::manifest(string location) -> string {
  vector<uint8_t> sector;
  if(directory::exists(location)) sector = readDataSectorBCD(location, manifestSector());
  if(file::exists(location)) sector = readDataSectorCUE(location, manifestSector());
  if(sector) return manifest(sector, location);
  return {};
}

auto CompactDisc::import(string filename) -> string {
  string location = {pathname, Location::prefix(filename), "/"};
  if(!directory::create(location)) return "output directory not writable";

  auto cdrom = vfs::cdrom::open(filename);
  if(!cdrom) return "failed to parse CUE sheet";

  if(auto fp = file::open({location, "cd.rom"}, file::mode::write)) {
    while(!cdrom->end()) fp.write(cdrom->read());
  }

  return {};
}

auto CompactDisc::readDataSectorBCD(string pathname, uint sectorID) -> vector<uint8_t> {
  auto fp = file::open({pathname, "cd.rom"}, file::mode::read);
  if(!fp) return {};

  vector<uint8_t> toc;
  toc.resize(96 * 7500);
  for(uint sector : range(7500)) {
    fp.read({toc.data() + 96 * sector, 96});
  }
  CD::Session session;
  session.decode(toc, 96);

  for(uint trackID : range(100)) {
    if(auto& track = session.tracks[trackID]) {
      if(!track.isData()) continue;
      if(auto index = track.index(1)) {
        vector<uint8_t> sector;
        sector.resize(2048);
        fp.seek(2448 * (abs(session.leadIn.lba) + index->lba + sectorID) + 16);
        fp.read({sector.data(), 2448});
        return sector;
      }
    }
  }

  return {};
}

auto CompactDisc::readDataSectorCUE(string filename, uint sectorID) -> vector<uint8_t> {
  Decode::CUE cuesheet;
  if(!cuesheet.load(filename)) return {};

  for(auto& file : cuesheet.files) {
    uint64_t offset = 0;
    auto location = string{Location::path(filename), file.name};

    if(file.type == "binary") {
      auto binary = file::open(location, nall::file::mode::read);
      if(!binary) continue;
      for(auto& track : file.tracks) {
        for(auto& index : track.indices) {
          uint sectorSize = 0;
          if(track.type == "mode1/2048") sectorSize = 2048;
          if(track.type == "mode1/2352") sectorSize = 2352;
          if(track.type == "mode2/2352") sectorSize = 2352;
          if(sectorSize && index.number == 1) {
            binary.seek(offset + (sectorSize * sectorID) + (sectorSize == 2352 ? 16 : 0));
            vector<uint8_t> sector;
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
