struct NeoGeo : Cartridge {
  auto name() -> string override { return "Neo Geo"; }
  auto extensions() -> vector<string> override { return {"ng"}; }
  auto read(string location, string match) -> vector<u8>;
  auto load(string location) -> bool override;
  auto save(string location) -> bool override;
  auto analyze(vector<u8>& p, vector<u8>& m, vector<u8>& c, vector<u8>& s, vector<u8>& v) -> string;
  auto endianSwap(vector<u8>&) -> void;
};

auto NeoGeo::read(string location, string match) -> vector<u8> {
  bool characterROM = match == "*.c*";
  bool programROM   = match == "*.p*";

  vector<u8> output;
  //parse Neo Geo ROM images in MAME ZIP file format:
  if(location.iendsWith(".zip")) {
    Decode::ZIP archive;
    if(archive.open(location)) {
      //find all files that match the requested pattern and then sort the results
      vector<string> filenames;
      for(auto& file : archive.file) {
        if(file.name.imatch(match)) filenames.append(file.name);
      }
      filenames.sort();

      //interleave charater ROM bitplanes
      if(characterROM) {
        vector<string> interleaved;
        for(u32 index = 0; index < filenames.size(); index += 2) interleaved.append(filenames[index]);
        for(u32 index = 1; index < filenames.size(); index += 2) interleaved.append(filenames[index]);
        filenames = interleaved;
      }

      //build the concatenated ROM image
      bool first = true;
      for(auto& filename : filenames) {
        for(auto& file : archive.file) {
          if(file.name != filename) continue;
          auto input = archive.extract(file);
          output.resize(output.size() + input.size());
          if(first && programROM) {
            first = false;
            if(input.size() > 1_MiB) {
              memory::copy(output.data() + output.size() - input.size(), input.data() + 1_MiB, input.size() - 1_MiB);
              memory::copy(output.data() + output.size() - (input.size() - 1_MiB), input.data(), 1_MiB);
              continue;
            }
            while(output.size() < 1_MiB) {
              memory::copy(output.data() + output.size() - input.size(), input.data(), input.size());
              output.resize(output.size() + min(1_MiB - output.size(), input.size()));
            }
          }
          memory::copy(output.data() + output.size() - input.size(), input.data(), input.size());
        }
      }
      if(programROM) endianSwap(output);
    }
  }
  return output;
}

auto NeoGeo::load(string location) -> bool {
  vector<u8> programROM;    //P ROM (68K CPU program)
  vector<u8> musicROM;      //M ROM (Z80 APU program)
  vector<u8> characterROM;  //C ROM (sprite and background character graphics)
  vector<u8> staticROM;     //S ROM (fix layer static graphics)
  vector<u8> voiceROM;      //V ROM (ADPCM voice samples)
  if(directory::exists(location)) {
    programROM   = file::read({location, "program.rom"});
    musicROM     = file::read({location, "music.rom"});
    characterROM = file::read({location, "character.rom"});
    staticROM    = file::read({location, "static.rom"});
    voiceROM     = file::read({location, "voice.rom"});
  } else if(file::exists(location)) {
    programROM   = NeoGeo::read(location, "*.p*");
    musicROM     = NeoGeo::read(location, "*.m*");
    characterROM = NeoGeo::read(location, "*.c*");
    staticROM    = NeoGeo::read(location, "*.s*");
    voiceROM     = NeoGeo::read(location, "*.v*");
  }
  if(!programROM  ) return false;
  if(!musicROM    ) return false;
  if(!characterROM) return false;
  if(!staticROM   ) return false;
  if(!voiceROM    ) return false;

  Hash::SHA256 hash;
  hash.input(programROM);
  hash.input(musicROM);
  hash.input(characterROM);
  hash.input(staticROM);
  hash.input(voiceROM);
  auto sha256 = hash.digest();

  this->location = location;
  this->manifest = analyze(programROM, musicROM, characterROM, staticROM, voiceROM);
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
  pak->append("voice.rom",     voiceROM);
  return true;
}

auto NeoGeo::save(string location) -> bool {
  auto document = BML::unserialize(manifest);

  return true;
}

auto NeoGeo::analyze(vector<u8>& p, vector<u8>& m, vector<u8>& c, vector<u8>& s, vector<u8>& v) -> string {
  string manifest;
  manifest += "game\n";
  manifest +={"  name:   ", Medium::name(location), "\n"};
  manifest +={"  title:  ", Medium::name(location), "\n"};
  manifest += "  board\n";
  manifest +={"    memory type=ROM size=0x", hex(p.size(), 8L), " content=Program\n"};
  manifest +={"    memory type=ROM size=0x", hex(m.size(), 8L), " content=Music\n"};
  manifest +={"    memory type=ROM size=0x", hex(c.size(), 8L), " content=Character\n"};
  manifest +={"    memory type=ROM size=0x", hex(s.size(), 8L), " content=Static\n"};
  manifest +={"    memory type=ROM size=0x", hex(v.size(), 8L), " content=Voice\n"};
  return manifest;
}

auto NeoGeo::endianSwap(vector<u8>& memory) -> void {
  for(u32 address = 0; address < memory.size(); address += 2) {
    swap(memory[address + 0], memory[address + 1]);
  }
}
