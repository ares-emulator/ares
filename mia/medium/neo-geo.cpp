struct NeoGeo : Cartridge {
  auto name() -> string override { return "Neo Geo"; }
  auto extensions() -> vector<string> override { return {"ng"}; }
  auto read(string location, string match) -> vector<u8>;
  auto load(string location) -> bool override;
  auto loadRoms(string location, string sectionName) -> vector<u8>;
  auto save(string location) -> bool override;
  auto analyze(vector<u8>& p, vector<u8>& m, vector<u8>& c, vector<u8>& s, vector<u8>& vA, vector<u8>& vB) -> string;
  auto endianSwap(vector<u8>& memory, u32 address = 0, int size = -1) -> void;

  Markup::Node info;
};

// TODO: Once we support more arcade systems, split this into a generic MAME medium
// Since we only have one arcade system at present, it's fine to keep it here.
auto NeoGeo::loadRoms(string location, string sectionName) -> vector<u8> {
  vector<u8> output;

  Decode::ZIP archive;
  if(!archive.open(location)) {
    return output;
  }

  string filename = {};
  vector<u8> input = {};
  u32 readOffset = 0;
  string loadType = {};

  for(auto section : info[{"game/", sectionName}]) {
    if(section.name() == "rom") {
      if(section["type"] && section["type"].string() != "continue") loadType = section["type"].string();

      if(section["name"]) {
        filename = section["name"].string().strip();
        if(filename) {
          input = {};
          for (auto &file: archive.file) {
            if (file.name != filename) continue;
            input = archive.extract(file);
            readOffset = 0;
            if (!input) return output;
          }
        }
      }

      auto writeOffset = section["offset"].natural();
      auto size = section["size"].natural();

      auto startIndex = 0;
      auto increment = 1;
      if(loadType == "load16_byte") {
        increment = 2;
        size *= 2;
        if(writeOffset & 1) {
          startIndex = 1;
          writeOffset--;
        }
      }

      if(output.size() < writeOffset + size) output.resize(writeOffset + size);

      for(auto index = startIndex; index < size; index += increment) {
        if(loadType == "fill") {
          output[index + writeOffset] = section["value"].natural();
          continue;
        }

        output[index + writeOffset] = input[readOffset++];
      }

      if(loadType == "load16_word_swap") {
        endianSwap(output, section["offset"].natural(), size);
      }
    }
  }

  return output;
}

auto NeoGeo::read(string location, string match) -> vector<u8> {
  // we expect mame style .zip rom images
  if(!location.iendsWith(".zip")) {}

  if(info) {
    if(match == "program.rom")   return loadRoms(location, "maincpu");
    if(match == "music.rom")     return loadRoms(location, "audiocpu");
    if(match == "character.rom") return loadRoms(location, "sprites");
    if(match == "static.rom")    return loadRoms(location, "fixed");
    if(match == "voice-a.rom")   return loadRoms(location, "ymsndadpcma");
    if(match == "voice-b.rom")   return loadRoms(location, "ymsndadpcmb");
  }

  return {};
}

auto NeoGeo::load(string location) -> bool {
  vector<u8> programROM;    //P ROM (68K CPU program)
  vector<u8> musicROM;      //M ROM (Z80 APU program)
  vector<u8> characterROM;  //C ROM (sprite and background character graphics)
  vector<u8> staticROM;     //S ROM (fix layer static graphics)
  vector<u8> voiceAROM;     //V ROM (ADPCM-A voice samples)
  vector<u8> voiceBROM;     //V ROM (ADPCM-B voice samples)

  this->info = BML::unserialize(manifestDatabaseArcade(Medium::name(location)));

  if(file::exists(location)) {
    programROM   = NeoGeo::read(location, "program.rom");
    musicROM     = NeoGeo::read(location, "music.rom");
    characterROM = NeoGeo::read(location, "character.rom");
    staticROM    = NeoGeo::read(location, "static.rom");
    voiceAROM    = NeoGeo::read(location, "voice-a.rom");
    voiceBROM    = NeoGeo::read(location, "voice-b.rom");
  }

  if(!programROM  ) return false;
  if(!musicROM    ) return false;
  if(!characterROM) return false;
  if(!staticROM   ) return false;
  if(!voiceAROM   ) return false;
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
  if(!document) return false;

  pak = new vfs::directory;
  pak->setAttribute("sha256",  sha256);
  pak->setAttribute("title",   document["game/title"].string());
  pak->append("manifest.bml",  manifest);
  pak->append("program.rom",   programROM);
  pak->append("music.rom",     musicROM);
  pak->append("character.rom", characterROM);
  pak->append("static.rom",    staticROM);
  pak->append("voice-a.rom",   voiceAROM);
  pak->append("voice-b.rom",   voiceBROM);
  return true;
}

auto NeoGeo::save(string location) -> bool {
  auto document = BML::unserialize(manifest);

  return true;
}

auto NeoGeo::analyze(vector<u8>& p, vector<u8>& m, vector<u8>& c, vector<u8>& s, vector<u8>& vA, vector<u8>& vB) -> string {
  string manifest;
  manifest += "game\n";
  manifest +={"  name:   ", Medium::name(location), "\n"};
  manifest +={"  title:  ", (info ? info["game/title"].string() : Medium::name(location)), "\n"};
  manifest += "  board\n";
  manifest +={"    memory type=ROM size=0x", hex( p.size(), 8L), " content=Program\n"};
  manifest +={"    memory type=ROM size=0x", hex( m.size(), 8L), " content=Music\n"};
  manifest +={"    memory type=ROM size=0x", hex( c.size(), 8L), " content=Character\n"};
  manifest +={"    memory type=ROM size=0x", hex( s.size(), 8L), " content=Static\n"};
  manifest +={"    memory type=ROM size=0x", hex(vA.size(), 8L), " content=VoiceA\n"};
  manifest +={"    memory type=ROM size=0x", hex(vB.size(), 8L), " content=VoiceB\n"};
  return manifest;
}

auto NeoGeo::endianSwap(vector<u8>& memory, u32 address, int size) -> void {
  if(size < 0) size = memory.size();
  for(u32 index = 0; index < size; index += 2) {
    swap(memory[address + index + 0], memory[address + index + 1]);
  }
}
