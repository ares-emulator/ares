struct NeoGeo : Cartridge {
  auto name() -> string override { return "Neo Geo"; }
  auto extensions() -> vector<string> override { return {"ng"}; }
  auto read(string location, string match) -> vector<u8>;
  auto load(string location) -> bool override;
  auto save(string location) -> bool override;
  auto analyze(vector<u8>& p, vector<u8>& m, vector<u8>& c, vector<u8>& s, vector<u8>& vA, vector<u8>& vB) -> string;
  auto endianSwap(vector<u8>&) -> void;
};

auto NeoGeo::read(string location, string match) -> vector<u8> {
  bool programROM = match == "program.rom";
  bool characterROM = match == "character.rom";

  vector<u8> output;

  // we expect mame style .zip rom images
  if(!location.iendsWith(".zip")) {
    return output;
  }

  Decode::ZIP archive;
  if(!archive.open(location)) {
    return output;
  }

  //find all files that match the correct pattern and then sort the results
  vector<string> filenames;

  // Program roms are either *.p* or *.ep*
  // secondary (banked) program roms are usually *.sp2
  if(match == "program.rom") {
    for (auto &file: archive.file) {
      if (file.name.imatch("*.p*")) filenames.append(file.name);
      if (file.name.imatch("*.ep*")) filenames.append(file.name);
      if (file.name.imatch("*.sp2")) filenames.append(file.name);
    }
  }

  if(match == "music.rom") {
    for (auto &file: archive.file) {
      if (file.name.imatch("*.m*")) filenames.append(file.name);
    }
  }

  if(match == "character.rom") {
    for (auto &file: archive.file) {
      if (file.name.imatch("*.c*")) filenames.append(file.name);
    }
  }

  // Thankfully, games only have one static rom.
  if(match == "static.rom") {
    for (auto &file: archive.file) {
      if (file.name.imatch("*.s1")) filenames.append(file.name);
    }
  }

  if(match == "voice-a.rom") {
    for (auto &file: archive.file) {
      if (file.name.imatch("*.v*")) filenames.append(file.name);
    }
  }

  if(match == "voice-b.rom") {
    for (auto &file: archive.file) {
      if (file.name.imatch("*.v2*")) filenames.append(file.name);
    }
  }

  filenames.sort();

  //interleave character ROM bitplanes
  if(characterROM) {
    vector<string> interleaved;
    for(u32 index = 0; index < filenames.size(); index += 2) interleaved.append(filenames[index]);
    for(u32 index = 1; index < filenames.size(); index += 2) interleaved.append(filenames[index]);
    filenames = interleaved;
  }

  // Special case for character roms: pad all character roms to the maximum sized crom
  // Test case: Twinkle Star Sprites
  if(characterROM) {
    vector<vector<u8>> roms;
    u32 maxRomSize = 0;

    // Step 1: load all croms and store the maximum size
    for(auto& filename : filenames) {
      for (auto &file: archive.file) {
        if (file.name != filename) continue;
        auto input = archive.extract(file);
        if (input.size() > maxRomSize) maxRomSize = input.size();
        roms.append(input);
      }
    }

    // Step 2: Load character roms into output, padding to maxRomSize
    for(auto& rom : roms) {
      u32 bytesCopied = 0;
      while(bytesCopied < maxRomSize) {
        output.resize(output.size() + rom.size());
        memory::copy(output.data() + output.size() - rom.size(), rom.data(), rom.size());
        bytesCopied += rom.size();
      }
    }

    return output;
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

  return output;
}

auto NeoGeo::load(string location) -> bool {
  vector<u8> programROM;    //P ROM (68K CPU program)
  vector<u8> musicROM;      //M ROM (Z80 APU program)
  vector<u8> characterROM;  //C ROM (sprite and background character graphics)
  vector<u8> staticROM;     //S ROM (fix layer static graphics)
  vector<u8> voiceAROM;     //V ROM (ADPCM-A voice samples)
  vector<u8> voiceBROM;     //V ROM (ADPCM-B voice samples)
  if(directory::exists(location)) {
    programROM   = file::read({location, "program.rom"});
    musicROM     = file::read({location, "music.rom"});
    characterROM = file::read({location, "character.rom"});
    staticROM    = file::read({location, "static.rom"});
    voiceAROM    = file::read({location, "voice-a.rom"});
    voiceBROM    = file::read({location, "voice-b.rom"});
  } else if(file::exists(location)) {
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
  manifest +={"  title:  ", Medium::name(location), "\n"};
  manifest += "  board\n";
  manifest +={"    memory type=ROM size=0x", hex( p.size(), 8L), " content=Program\n"};
  manifest +={"    memory type=ROM size=0x", hex( m.size(), 8L), " content=Music\n"};
  manifest +={"    memory type=ROM size=0x", hex( c.size(), 8L), " content=Character\n"};
  manifest +={"    memory type=ROM size=0x", hex( s.size(), 8L), " content=Static\n"};
  manifest +={"    memory type=ROM size=0x", hex(vA.size(), 8L), " content=VoiceA\n"};
  manifest +={"    memory type=ROM size=0x", hex(vB.size(), 8L), " content=VoiceB\n"};
  return manifest;
}

auto NeoGeo::endianSwap(vector<u8>& memory) -> void {
  for(u32 address = 0; address < memory.size(); address += 2) {
    swap(memory[address + 0], memory[address + 1]);
  }
}
