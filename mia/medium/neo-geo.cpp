// Decryption algorithms heavily based on/borrowed MAME
// See https://github.com/mamedev/mame/tree/master/src/devices/bus/neogeo
// and the MAME section in LICENCE in the ares license text.

struct NeoGeo : Mame {
  auto name() -> string override { return "Neo Geo"; }
  auto extensions() -> std::vector<string> override { return {"ng"}; }
  auto database() -> Database override;
  auto read(string location, string match) -> AssemblyResult;
  auto load(string location) -> LoadResult override;
  auto board() -> string;
  auto save(string location) -> bool override;
  auto analyze(std::vector<u8>& p, std::vector<u8>& m, std::vector<u8>& c, std::vector<u8>& s, std::vector<u8>& vA, std::vector<u8>& vB) -> string;
  Markup::Node info;
};

auto NeoGeo::database() -> Database {
  return Arcade().database();
}

auto NeoGeo::read(string location, string match) -> AssemblyResult {
  // we expect mame style .zip rom images
  if(!location.iendsWith(".zip")) {}

  if(info) {
    auto tryRegion = [&](string region) -> AssemblyResult {
      if(info[{"game/cslot1-", region}]) return loadRoms(location, info, {"cslot1-", region});
      return loadRoms(location, info, region);
    };

    if(match == "program.rom")   return tryRegion("maincpu");
    if(match == "character.rom") return tryRegion("sprites");
    if(match == "static.rom")    return tryRegion("fixed");
    if(match == "voice-a.rom")   return tryRegion("ymsnd-adpcma");
    if(match == "voice-b.rom")   return tryRegion("ymsnd-adpcmb");
    if(match == "music.rom") {
      if(info["game/cslot1-audiocpu"] || info["game/audiocpu"]) {
        auto music = tryRegion("audiocpu");
        if(!music || !music.data.empty()) return music;
      }
      return tryRegion("audiocrypt");
    }
  }

  return {};
}

auto NeoGeo::load(string location) -> LoadResult {
  std::vector<u8> programROM;    //P ROM (68K CPU program)
  std::vector<u8> musicROM;      //M ROM (Z80 APU program)
  std::vector<u8> characterROM;  //C ROM (sprite and background character graphics)
  std::vector<u8> staticROM;     //S ROM (fix layer static graphics)
  std::vector<u8> voiceAROM;     //V ROM (ADPCM-A voice samples)
  std::vector<u8> voiceBROM;     //V ROM (ADPCM-B voice samples)

  auto source = database();
  if(!source.list) return {databaseNotFound, "Arcade.bml"};
  info = {};
  auto gameName = Medium::name(location);
  for(auto node : source.list) {
    if(node["board"].string() != "snk/neogeo") continue;
    if(!node["name"].string().iequals(gameName)) continue;
    info = BML::unserialize(BML::serialize(node));
    break;
  }
  if(!info) return romNotFoundInDatabase;

  auto programResult   = NeoGeo::read(location, "program.rom");
  auto musicResult     = NeoGeo::read(location, "music.rom");
  auto characterResult = NeoGeo::read(location, "character.rom");
  auto staticResult    = NeoGeo::read(location, "static.rom");
  auto voiceAResult    = NeoGeo::read(location, "voice-a.rom");
  auto voiceBResult    = NeoGeo::read(location, "voice-b.rom");

  string invalidRomInfo = "Ensure your ROM is in a MAME-compatible .zip format.";

  if(!programResult)   return {invalidROM, assemblyError(programResult)};
  if(!musicResult)     return {invalidROM, assemblyError(musicResult)};
  if(!characterResult) return {invalidROM, assemblyError(characterResult)};
  if(!staticResult)    return {invalidROM, assemblyError(staticResult)};
  if(!voiceAResult)    return {invalidROM, assemblyError(voiceAResult)};
  if(!voiceBResult)    return {invalidROM, assemblyError(voiceBResult)};

  programROM   = std::move(programResult.data);
  musicROM     = std::move(musicResult.data);
  characterROM = std::move(characterResult.data);
  staticROM    = std::move(staticResult.data);
  voiceAROM    = std::move(voiceAResult.data);
  voiceBROM    = std::move(voiceBResult.data);

  if(programROM.empty()  ) return { invalidROM, invalidRomInfo };
  if(musicROM.empty()    ) return { invalidROM, invalidRomInfo };
  if(characterROM.empty()) return { invalidROM, invalidRomInfo };
  if(staticROM.empty()   ) return { invalidROM, invalidRomInfo };
  if(voiceAROM.empty()   ) return { invalidROM, invalidRomInfo };
  //voiceB is optional

  Hash::SHA256 hash;
  hash.input(programROM);
  hash.input(musicROM);
  hash.input(characterROM);
  hash.input(staticROM);
  hash.input(voiceAROM);
  hash.input(voiceBROM);
  auto sha256 = hash.digest();

  this->location = location;
  this->manifest = analyze(programROM, musicROM, characterROM, staticROM, voiceAROM, voiceBROM);
  auto document = BML::unserialize(manifest);
  if(!document) return couldNotParseManifest;

  pak = std::make_shared<vfs::directory>();
  pak->setAttribute("sha256",  sha256);
  pak->setAttribute("title",   document["game/title"].string());
  pak->setAttribute("board",   document["game/board"].string());
  pak->append("manifest.bml",  manifest);
  pak->append("program.rom",   programROM);
  pak->append("music.rom",     musicROM);
  pak->append("character.rom", characterROM);
  pak->append("static.rom",    staticROM);
  pak->append("voice-a.rom",   voiceAROM);
  pak->append("voice-b.rom",   voiceBROM);
  return successful;
}

auto NeoGeo::save(string location) -> bool {
  auto document = BML::unserialize(manifest);

  Pak::save("dips.bml", ".dip");

  return true;
}

auto NeoGeo::board() -> string {
  if(info) {
    for (auto element: info["game"]) {
      if(element.name() == "feature" && element["name"].string() == "slot") return element["value"].string();
    }
  }

  return "rom";
}

auto NeoGeo::analyze(std::vector<u8>& p, std::vector<u8>& m, std::vector<u8>& c, std::vector<u8>& s, std::vector<u8>& vA, std::vector<u8>& vB) -> string {
  string manifest;

  manifest += "game\n";
  manifest +={"  name:   ", Medium::name(location), "\n"};
  manifest +={"  title:  ", (info ? info["game/title"].string() : Medium::name(location)), "\n"};
  manifest +={"  board:  ", board(), "\n"};
  manifest +={"    memory type=ROM size=0x", hex( p.size(), 8L), " content=Program\n"};
  manifest +={"    memory type=ROM size=0x", hex( m.size(), 8L), " content=Music\n"};
  manifest +={"    memory type=ROM size=0x", hex( c.size(), 8L), " content=Character\n"};
  manifest +={"    memory type=ROM size=0x", hex( s.size(), 8L), " content=Static\n"};
  manifest +={"    memory type=ROM size=0x", hex(vA.size(), 8L), " content=VoiceA\n"};
  manifest +={"    memory type=ROM size=0x", hex(vB.size(), 8L), " content=VoiceB\n"};
  return manifest;
}
