struct VsSystemDatabase {
  enum class Status : u32 {
    Successful,
    NotFound,
    CouldNotParse,
  };

  auto load() -> Status;
  auto database() -> Database;
  auto manifest(string name) -> string;
  auto document(string name) -> Markup::Node;
};

auto VsSystemDatabase::load() -> Status {
  for(auto& database : Media::databases) {
    if(database.name == "VsSystem") return Status::Successful;
  }

  auto databaseFile = locate("Database/VsSystem.bml");
  if(!inode::exists(databaseFile)) return Status::NotFound;

  Database database;
  database.name = "VsSystem";
  database.list = BML::unserialize(file::read(databaseFile));
  if(!database.list) return Status::CouldNotParse;

  Media::databases.push_back(std::move(database));
  return Status::Successful;
}

auto VsSystemDatabase::database() -> Database {
  if(load() != Status::Successful) return {};
  for(auto& database : Media::databases) {
    if(database.name == "VsSystem") return database;
  }
  return {};
}

auto VsSystemDatabase::manifest(string name) -> string {
  auto source = database();
  for(auto node : source.list) {
    if(node["name"].string().iequals(name)) return BML::serialize(node);
  }
  return {};
}

auto VsSystemDatabase::document(string name) -> Markup::Node {
  return BML::unserialize(manifest(name));
}

struct Arcade : Mame {
  auto name() -> string override { return "Arcade"; }
  auto extensions() -> std::vector<string> override { return {}; }
  auto database() -> Database override;
  auto load(string location) -> LoadResult override;
  auto save(string location) -> bool override;

private:
  auto loadGeneric(string location, Markup::Node& document) -> LoadResult;
  auto loadVs(string location, Markup::Node& document) -> LoadResult;
};

auto Arcade::database() -> Database {
  auto combined = Medium::database();
  combined.name = "Arcade";
  combined.list = combined.list.clone();

  VsSystemDatabase vsDatabase;
  auto vs = vsDatabase.database();
  for(auto node : vs.list) {
    if(node["hardware/topology"].string() != "unisystem") continue;

    auto nodeName = node["name"].string();
    bool duplicate = false;
    for(auto existing : combined.list) {
      if(existing["name"].string().iequals(nodeName)) {
        duplicate = true;
        break;
      }
    }
    if(!duplicate) combined.list.append(node.clone());
  }

  return combined;
}

auto Arcade::load(string location) -> LoadResult {
  manifest = {};
  auto shortName = Medium::name(location);
  auto genericAvailable = Medium::loadDatabase();
  if(genericAvailable) manifest = manifestDatabaseArcade(shortName);

  if(manifest) {
    auto document = BML::unserialize(manifest);
    if(!document) return couldNotParseManifest;
    if(document["game/board"].string() == "snk/neogeo") {
      auto arcadeManifest = manifest;
      auto neoGeo = Medium::create("Neo Geo");
      auto result = neoGeo->load(location);
      if(result != successful) return result;
      neoGeo->pak->setAttribute("arcade", "neogeo");
      neoGeo->pak->append("arcade.bml", arcadeManifest);
      neoGeo->pak->append("dips.bml", 0);
      this->location = neoGeo->location;
      this->manifest = arcadeManifest;
      this->pak = neoGeo->pak;
      Pak::load("dips.bml", ".dip");
      return successful;
    }
    auto result = document["game/board"].string() == "nintendo/vs" ? loadVs(location, document) : loadGeneric(location, document);
    if(result == successful) {
      pak->append("dips.bml", 0);
      Pak::load("dips.bml", ".dip");
    }
    return result;
  }

  VsSystemDatabase vsDatabase;
  auto vsStatus = vsDatabase.load();
  if(vsStatus == VsSystemDatabase::Status::CouldNotParse) {
    return {couldNotParseManifest, "VsSystem.bml"};
  }

  if(vsStatus == VsSystemDatabase::Status::Successful) {
    manifest = vsDatabase.manifest(shortName);
    if(manifest) {
      auto document = BML::unserialize(manifest);
      if(!document) return couldNotParseManifest;
      auto result = loadVs(location, document);
      if(result == successful) {
        pak->append("dips.bml", 0);
        Pak::load("dips.bml", ".dip");
      }
      return result;
    }
  }

  if(!genericAvailable && vsStatus == VsSystemDatabase::Status::NotFound) {
    return {databaseNotFound, "Arcade.bml and VsSystem.bml"};
  }
  return romNotFoundInDatabase;
}

auto Arcade::loadGeneric(string location, Markup::Node& document) -> LoadResult {
  auto resolver = [&](string parent) -> Markup::Node {
    return BML::unserialize(manifestDatabaseArcade(parent));
  };

  if(document["game/board"].string() == "sega/segae") {
    auto rom = loadRoms(location, document, "maincpu", resolver);
    if(!rom) return {invalidROM, assemblyError(rom)};
    if(rom.data.empty()) return {invalidROM, "Ensure your ROM is in a MAME-compatible .zip format."};

    auto key = loadRoms(location, document, "maincpu-key", resolver);
    if(!key) return {invalidROM, assemblyError(key)};

    this->location = location;
    pak = std::make_shared<vfs::directory>();
    pak->setAttribute("board", document["game/board"].string());
    pak->setAttribute("name", document["game/name"].string());
    pak->setAttribute("title", document["game/title"].string());
    pak->setAttribute("cpu", document["game/cpu"].string());
    pak->setAttribute("rotation", document["game/rotation"].string());
    pak->append("manifest.bml", manifest);
    pak->append("program.rom", rom.data);
    if(!key.data.empty()) pak->append("cpu.key", key.data);
    return successful;
  }

  // Sega SG-1000 based arcade
  if(document["game/board"].string() == "sega/sg1000a") {
    auto rom = loadRoms(location, document, "maincpu", resolver);
    if(!rom) return {invalidROM, assemblyError(rom)};
    if(rom.data.empty()) {
      return {invalidROM, "Ensure your ROM is in a MAME-compatible .zip format."};
    }

    this->location = location;

    pak = std::make_shared<vfs::directory>();
    pak->setAttribute("board",  document["game/board" ].string());
    pak->setAttribute("name",   document["game/name" ].string());
    pak->setAttribute("title",  document["game/title"].string());
    pak->setAttribute("region", document["game/region"].string());
    pak->setAttribute("cpu",    document["game/cpu"].string());
    pak->append("manifest.bml", manifest);
    pak->append("program.rom",  rom.data);

    return successful;
  }

  if(document["game/board"].string() == "konami/konamigv") {
    auto bios = loadRoms(location, document, "maincpu-rom", resolver);
    if(!bios) return {invalidROM, assemblyError(bios)};
    if(bios.data.empty()) return {invalidROM, "Ensure your ROM is in a MAME-compatible .zip format and that konamigv.zip is available."};
    auto eeprom = loadRoms(location, document, "eeprom", resolver);
    if(!eeprom) return {invalidROM, assemblyError(eeprom)};

    this->location = location;
    pak = std::make_shared<vfs::directory>();
    pak->setAttribute("board", document["game/board"].string());
    pak->setAttribute("name", document["game/name"].string());
    pak->setAttribute("title", document["game/title"].string());
    pak->setAttribute("region", "NTSC-J");
    pak->append("manifest.bml", manifest);
    pak->append("bios.rom", bios.data);
    for(auto nvram : document["game"].find("nvram")) {
      auto name = nvram["name"].string();
      if(!name) continue;
      pak->append(name, std::vector<u8>(nvram["size"].natural(), nvram["fill"].natural()));
      Pak::load(name, Location::suffix(name));
    }
    if(!eeprom.data.empty()) pak->append("eeprom.rom", eeprom.data);
    pak->append("save.eeprom", eeprom.data.empty() ? std::vector<u8>(128, 0xff) : eeprom.data);
    Pak::load("save.eeprom", ".eeprom");

    auto flash = loadRoms(location, document, "flash", resolver);
    if(!flash) return {invalidROM, assemblyError(flash)};
    if(!flash.data.empty()) pak->append("flash.rom", flash.data);

    auto disks = document["game"].find("disk");
    if(!disks.empty()) {
      auto disc = locateDisk(location, document, resolver);
      if(!disc) {
        return {romNotFound, {"CD image not found. Expected ", Location::dir(location), document["game/name"].string(), "/", disks.front()["name"].string(), ".chd"}};
      }
      auto cdrom = vfs::cdrom::open(disc);
      if(!cdrom) return {invalidROM, {"CD image ", Location::file(disc), " could not be read."}};
      pak->append("cd.rom", cdrom);
    }
    return successful;
  }

  // Aleck 64
  if(document["game/board"].string() == "nintendo/aleck64") {
    auto rom = loadRoms(location, document, "user2", resolver);
    if(!rom) return {invalidROM, assemblyError(rom)};
    if(rom.data.empty()) {
      return {invalidROM, "Ensure your ROM is in a MAME-compatible .zip format."};
    }

    // MAME stores ROMs in byte-swapped (v64) format, but we need native big-endian (z64).
    if(!endianSwap(rom.data)) {
      return {invalidROM, "Aleck64 program ROM has an invalid byte-swap extent."};
    }

    auto pif = loadRoms(location, document, "user1", resolver);
    if(!pif) return {invalidROM, assemblyError(pif)};
    if(pif.data.empty()) return {
      invalidROM,
      "Ensure your ROM is in a MAME-compatible .zip format and that the Aleck64 pif ROM is available."
    };

    this->location = location;

    pak = std::make_shared<vfs::directory>();
    pak->setAttribute("board",  document["game/board" ].string());
    pak->setAttribute("name",   document["game/name" ].string());
    pak->setAttribute("title",  document["game/title"].string());
    pak->setAttribute("region", document["game/region"].string());
    pak->setAttribute("cic",    "CIC-NUS-5101");
    pak->append("manifest.bml", manifest);
    pak->append("program.rom", rom.data);
    pak->append("pif.aleck64.rom", pif.data);

    return successful;
  }

  return otherError;
}

auto Arcade::loadVs(string location, Markup::Node& document) -> LoadResult {
  auto topology = document["game/hardware/topology"].string();
  if(topology == "unsupported") {
    auto reason = document["game/hardware/reason"].string();
    if(reason != "dualsystem"
    && reason != "raid-protection"
    && reason != "bootleg-z80") {
      return {invalidROM, "Invalid Vs. System metadata field: game/hardware/reason. "};
    }
    return {unsupportedMedia, reason};
  }

  VsSystemDatabase vsDatabase;
  auto resolver = [&](string parent) -> Markup::Node {
    return vsDatabase.document(parent);
  };

  auto program = loadRoms(location, document, "prg", resolver);
  if(!program) return {invalidROM, assemblyError(program)};

  auto mapper = document["game/hardware/mapper"].string();
  AssemblyResult character;
  bool hasCharacter = (bool)document["game/gfx1"];
  if(hasCharacter) {
    character = loadRoms(location, document, "gfx1", resolver);
    if(!character) return {invalidROM, assemblyError(character)};
  }

  auto palette = loadRoms(location, document, "ppu1-palette", resolver);
  if(!palette) return {invalidROM, assemblyError(palette)};

  this->location = location;
  pak = std::make_shared<vfs::directory>();
  pak->setAttribute("board",      "nintendo/vs");
  pak->setAttribute("name",       document["game/name"].string());
  pak->setAttribute("title",      document["game/title"].string());
  pak->setAttribute("region",     "NTSC");
  pak->setAttribute("system",     "Vs. UniSystem");
  pak->setAttribute("ppu",        document["game/hardware/ppu"].string());
  pak->setAttribute("mapper",     mapper);
  pak->setAttribute("protection", document["game/hardware/protection"].string());
  pak->setAttribute("input",      document["game/hardware/input"].string());
  pak->append("manifest.bml", manifest);
  pak->append("program.rom", program.data);
  if(hasCharacter) pak->append("character.rom", character.data);
  pak->append("palette.rom", palette.data);
  return successful;
}

auto Arcade::save(string location) -> bool {
  Pak::save("dips.bml", ".dip");
  Pak::save("save.eeprom", ".eeprom");
  if(auto document = BML::unserialize(manifest)) {
    for(auto nvram : document["game"].find("nvram")) {
      if(auto name = nvram["name"].string()) Pak::save(name, Location::suffix(name));
    }
  }
  return true;
}
