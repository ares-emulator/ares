namespace Media {
  std::vector<Database> databases;
  #include "mame.cpp"
  #include "arcade.cpp"
  #include "atari-2600.cpp"
  #include "colecovision.cpp"
  #include "myvision.cpp"
  #include "famicom.cpp"
  #include "famicom-disk-system.cpp"
  #include "game-boy.cpp"
  #include "game-boy-color.cpp"
  #include "game-boy-advance.cpp"
  #include "master-system.cpp"
  #include "game-gear.cpp"
  #include "mega-drive.cpp"
  #include "mega-32x.cpp"
  #include "mega-cd.cpp"
  #include "mega-ld.cpp"
  #include "msx.cpp"
  #include "msx2.cpp"
  #include "neo-geo.cpp"
  #include "neo-geo-pocket.cpp"
  #include "neo-geo-pocket-color.cpp"
  #include "nintendo-64.cpp"
  #include "nintendo-64dd.cpp"
  #include "pc-engine.cpp"
  #include "pc-engine-cd.cpp"
  #include "pc-engine-ld.cpp"
  #include "saturn.cpp"
  #include "supergrafx.cpp"
  #include "playstation.cpp"
  #include "sg-1000.cpp"
  #include "sc-3000.cpp"
  #include "super-famicom.cpp"
  #include "bs-memory.cpp"
  #include "sufami-turbo.cpp"
  #include "wonderswan.cpp"
  #include "wonderswan-color.cpp"
  #include "pocket-challenge-v2.cpp"
  #include "zx-spectrum.cpp"
}

//auto Medium::construct() -> void {
//  database = BML::unserialize(file::read(locate({"Database/", name(), ".bml"})));
//  pathname = {Path::user(), "Emulation/", name(), "/"};
//}

auto Medium::create(string name) -> std::shared_ptr<Pak> {
  if(name == "Arcade") return std::make_shared<Media::Arcade>();
  if(name == "Atari 2600") return std::make_shared<Media::Atari2600>();
  if(name == "ColecoVision") return std::make_shared<Media::ColecoVision>();
  if(name == "MyVision") return std::make_shared<Media::MyVision>();
  if(name == "Famicom") return std::make_shared<Media::Famicom>();
  if(name == "Famicom Disk System") return std::make_shared<Media::FamicomDiskSystem>();
  if(name == "Game Boy") return std::make_shared<Media::GameBoy>();
  if(name == "Game Boy Color") return std::make_shared<Media::GameBoyColor>();
  if(name == "Game Boy Advance") return std::make_shared<Media::GameBoyAdvance>();
  if(name == "Master System") return std::make_shared<Media::MasterSystem>();
  if(name == "Game Gear") return std::make_shared<Media::GameGear>();
  if(name == "Mega Drive") return std::make_shared<Media::MegaDrive>();
  if(name == "Mega 32X") return std::make_shared<Media::Mega32X>();
  if(name == "Mega CD") return std::make_shared<Media::MegaCD>();
  if(name == "Mega LD") return std::make_shared<Media::MegaLD>();
  if(name == "MSX") return std::make_shared<Media::MSX>();
  if(name == "MSX2") return std::make_shared<Media::MSX2>();
  if(name == "Neo Geo") return std::make_shared<Media::NeoGeo>();
  if(name == "Neo Geo Pocket") return std::make_shared<Media::NeoGeoPocket>();
  if(name == "Neo Geo Pocket Color") return std::make_shared<Media::NeoGeoPocketColor>();
  if(name == "Nintendo 64") return std::make_shared<Media::Nintendo64>();
  if(name == "Nintendo 64DD") return std::make_shared<Media::Nintendo64DD>();
  if(name == "PC Engine") return std::make_shared<Media::PCEngine>();
  if(name == "PC Engine CD") return std::make_shared<Media::PCEngineCD>();
  if(name == "PC Engine LD") return std::make_shared<Media::PCEngineLD>();
  if(name == "Saturn") return std::make_shared<Media::Saturn>();
  if(name == "SuperGrafx") return std::make_shared<Media::SuperGrafx>();
  if(name == "PlayStation") return std::make_shared<Media::PlayStation>();
  if(name == "SG-1000") return std::make_shared<Media::SG1000>();
  if(name == "SC-3000") return std::make_shared<Media::SC3000>();
  if(name == "Super Famicom") return std::make_shared<Media::SuperFamicom>();
  if(name == "BS Memory") return std::make_shared<Media::BSMemory>();
  if(name == "Sufami Turbo") return std::make_shared<Media::SufamiTurbo>();
  if(name == "WonderSwan") return std::make_shared<Media::WonderSwan>();
  if(name == "WonderSwan Color") return std::make_shared<Media::WonderSwanColor>();
  if(name == "Pocket Challenge V2") return std::make_shared<Media::PocketChallengeV2>();
  if(name == "ZX Spectrum") return std::make_shared<Media::ZXSpectrum>();
  return {};
}

auto Medium::loadDatabase() -> bool {
  //load the database on the first time it's needed for a given media type
  for(auto& database : Media::databases) {
    if(database.name == name()) return true;
  }

  Database database;
  database.name = name();
  auto databaseFile = locate({"Database/", name(), ".bml"});
  if(inode::exists(databaseFile)) {
    database.list = BML::unserialize(file::read(databaseFile));
    Media::databases.push_back(std::move(database));
    return true;
  }

  return false;
}

//Retrieve all entries in game database
auto Medium::database() -> Database {
  loadDatabase();

  //search the database for a given sha256 game entry
  for(auto& database : Media::databases) {
    if (database.name == name()) {
      return database;
    }
  }

  return {};
}

//search game database for manifest, if one exists
auto Medium::manifestDatabase(string sha256) -> string {
  loadDatabase();

  //search the database for a given sha256 game entry
  for(auto& database : Media::databases) {
    if(database.name == name()) {
      for(auto node : database.list) {
        if(node["sha256"].string() == sha256) {
          return BML::serialize(node);
        }
      }
    }
  }

  //database or game entry not found
  return {};
}

//search game database for manifest, if one exists
auto Medium::manifestDatabaseArcade(string rom) -> string {
  loadDatabase();

  //search the database for a given named game entry
  for(auto& database : Media::databases) {
    if(database.name == name()) {
      for(auto node : database.list) {
        if(node["name"].string().iequals(rom)) {
          return BML::serialize(node);
        }
      }
    }
  }

  //database or game entry not found
  return {};
}

//

//audio CD fallback
auto CompactDisc::manifestAudio(string location) -> string {
  string manifest;
  manifest += "game\n";
  manifest +={"  name:  ", Medium::name(location), "\n"};
  manifest +={"  title: ", Medium::name(location), "\n"};
  manifest += "  audio\n";
  return manifest;
}

auto CompactDisc::isAudioCd(string pathname) -> bool {
  auto fp = file::open({pathname, "cd.rom"}, file::mode::read);
  if(!fp) return {};

  std::vector<u8> toc;
  toc.resize(96 * 7500);
  for(u32 sector : range(7500)) {
    fp.read({toc.data() + 96 * sector, 96});
  }
  CD::Session session;
  session.decode(toc, 96);

  //If the disc contains no data tracks, we are an audio cd
  for(u32 trackID : range(100)) {
    if(auto& track = session.tracks[trackID]) {
      if(track.isData()) return false;
    }
  }

  return true;
}

auto CompactDisc::readDataSector(string pathname, u32 sectorID) -> std::vector<u8> {
  if(pathname.iendsWith(".bcd")) {
    return readDataSectorBCD(pathname, sectorID);
  }
  if(pathname.iendsWith(".cue")) {
    return readDataSectorCUE(pathname, sectorID);
  }
#if defined(ARES_ENABLE_CHD)
  if(pathname.iendsWith(".chd")) {
    return readDataSectorCHD(pathname, sectorID);
  }
#endif
  return {};
}

auto CompactDisc::readDataSectorBCD(string pathname, u32 sectorID) -> std::vector<u8> {
  auto fp = file::open({pathname, "cd.rom"}, file::mode::read);
  if(!fp) return {};

  std::vector<u8> toc;
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
        std::vector<u8> sector;
        sector.resize(2448);
        fp.seek(2448 * (abs(session.leadIn.lba) + index->lba + sectorID) + 16);
        fp.read({sector.data(), 2448});
        return sector;
      }
    }
  }

  return {};
}

auto CompactDisc::readDataSectorCUE(string filename, u32 sectorID) -> std::vector<u8> {
  Decode::CUE cuesheet;
  if(!cuesheet.load(filename, nullptr, nullptr)) return {};

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
            std::vector<u8> sector;
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

#if defined(ARES_ENABLE_CHD)
auto CompactDisc::readDataSectorCHD(string filename, u32 sectorID) -> std::vector<u8> {
  Decode::CHD chd;
  if(!chd.load(filename)) return {};

  // Find the first data track within the chd
  for (auto& track : chd.tracks) {
    if(track.type == "AUDIO") continue;
    for(auto& index : track.indices) {
      if (index.number != 1) continue;

      // Read the sector from CHD and extract the user data portion (2048 bytes)
      auto sector = chd.read(index.lba + sectorID);
      std::vector<u8> output;
      output.resize(2048);

      if (sector.size() == 2048) {
        memory::copy(output.data(), output.size(), sector.data(), output.size());
        return output;
      }

      memory::copy(output.data(), output.size(), sector.data() + 16, output.size());
      return output;
    }
  }

  return {};
}
#endif

auto LaserDisc::readDataSector(string mmiPath, string cuePath, u32 sectorID) -> std::vector<u8> {
  std::unique_ptr<Decode::ZIP> archive = std::make_unique<Decode::ZIP>();
  if (!archive->open(mmiPath)) return {};
  Decode::CUE cuesheet;
  if(!cuesheet.load(mmiPath, archive.get(), &archive->findFile(cuePath).get())) return {};

  for(auto& file : cuesheet.files) {
    u64 offset = 0;
    if(file.type == "binary") {
      auto filePathInArchive = file.archiveFolder;
      filePathInArchive.append(file.name);
      auto fileEntry = archive->findFile(filePathInArchive);
      if(!fileEntry) continue;
      std::span<const u8> rawDataView;
      std::vector<u8> rawDataBuffer;
      if (archive->isDataUncompressed(*fileEntry)) {
        rawDataView = archive->dataViewIfUncompressed(*fileEntry);
      } else {
        auto tmp = archive->extract(*fileEntry);
        rawDataBuffer.resize(tmp.size());
        if(!tmp.empty()) memcpy(rawDataBuffer.data(), tmp.data(), tmp.size());
        rawDataView = {rawDataBuffer.data(), rawDataBuffer.size()};
      }
      for(auto& track : file.tracks) {
        for(auto& index : track.indices) {
          u32 sectorSize = 0;
          if(track.type == "mode1/2048") sectorSize = 2048;
          if(track.type == "mode1/2352") sectorSize = 2352;
          if(track.type == "mode2/2352") sectorSize = 2352;
          if(sectorSize && index.number == 1) {
            size_t readPos = offset + (sectorSize * sectorID) + (sectorSize == 2352 ? 16 : 0);
            std::vector<u8> sector;
            sector.resize(2048);
            memcpy(sector.data(), rawDataView.data() + readPos, sector.size());
            return sector;
          }
          offset += track.sectorSize() * index.sectorCount();
        }
      }
    }
  }

  return {};
}
