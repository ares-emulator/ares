struct MSX : Cartridge {
  auto name() -> string override { return "MSX"; }
  auto extensions() -> std::vector<string> override { return {"msx", "rom", "wav", "tzx", "tsx", "cas"}; }
  auto load(string location) -> LoadResult override;
  auto save(string location) -> bool override;
  auto analyze(std::vector<u8>& rom) -> string;
  auto loadTape(string location) -> LoadResult;
  auto loadCas(string location) -> LoadResult;
  auto loadTzx(string location) -> LoadResult;
  auto saveWav(string location) -> bool;
  auto analyzeTape(string location) -> string;
};

auto MSX::load(string location) -> LoadResult {
  if(location.iendsWith(".wav") || location.iendsWith(".tzx") || location.iendsWith(".tsx") || location.iendsWith(".cas")) {
    return loadTape(location);
  }

  std::vector<u8> rom;
  if(directory::exists(location)) {
    append(rom, {location, "program.rom"});
  } else if(file::exists(location)) {
    rom = Cartridge::read(location);
  }
  if(rom.empty()) return romNotFound;

  this->sha256   = Hash::SHA256(rom).digest();
  this->location = location;
  auto foundDatabase = Medium::loadDatabase();
  if(!foundDatabase) return { databaseNotFound, "MSX.bml" };
  this->manifest = Medium::manifestDatabase(sha256);
  if(!manifest) manifest = analyze(rom);
  auto document = BML::unserialize(manifest);
  if(!document) return couldNotParseManifest;

  pak = std::make_shared<vfs::directory>();
  pak->setAttribute("title",  document["game/title"].string());
  pak->setAttribute("region", document["game/region"].string());
  pak->setAttribute("board",  document["game/board"].string());
  pak->setAttribute("vauspaddle", (bool)document["game/vauspaddle"]);
  pak->append("manifest.bml", manifest);
  pak->append("program.rom",  rom);

  return successful;
}

auto MSX::save(string location) -> bool {
  auto document = BML::unserialize(manifest);
  if(!pak) return false;

  if(pak->attribute("tape").boolean()) {
    if(!pak->attribute("modified").boolean()) return true;

    if(file::exists(location) && location.iendsWith(".wav")) {
      return saveWav(location);
    }

    if(file::exists(location) && (location.iendsWith(".tzx") || location.iendsWith(".tsx") || location.iendsWith(".cas"))) {
      return saveWav(saveLocation(location, "program.tape", ".wav"));
    }
  }

  return true;
}

auto MSX::analyze(std::vector<u8>& rom) -> string {
  if(location.iendsWith(".wav")) {
    return analyzeTape(location);
  }

  string hash   = Hash::SHA256(rom).digest();
  string board = "Linear";
  bool vauspaddle = false;

  // Roms <= 16KB are most likely (but not always) mirrored
  // This is because MSX looks at 0x4000 for the rom header
  if (rom.size() <= 0x4000) {
    board = "Mirrored";
  }

  // 16KB roms may be mapped at 0x8000 instead of 0x4000
  // We can check for this by checking which range the init
  // and text fields of the cartridge header fall into
  if ((rom.size() == 0x4000) && (rom[0] == 'A') && (rom[1] == 'B')) {
    n16 init = rom[2] | (rom[3] << 8);
    n16 text = rom[8] | (rom[9] << 8);

    bool textHas8000base = text.bit(14, 15) == 2;
    bool hasNoInitVector = init == 0;
    bool initHas8000base = init.bit(14, 15) == 2;
    bool init8000BaseHasRet = initHas8000base && rom[init & (rom.size() - 1)] == 0xC9;

    if (textHas8000base && (hasNoInitVector || init8000BaseHasRet)) {
      board = "LinearPage2";
    }
  }  
  
  // If the rom is too big to be linear, attempt to guess the mapper
  // based on the number of times specific banking instructions occur 
  // in the binary
  if(rom.size() > 0x10000) {
    const auto ASC16      = 0;
    const auto ASC8       = 1;
    const auto KONAMI     = 2;
    const auto KONAMI_SCC = 3;
    int bankingCount[4] = {0,0,0,0}; 

    for(auto i : range(rom.size() - 3)) {
      if (rom[i] == 0x32) { // ld(nn),a 
        u16 value = rom[i + 1] + (rom[i + 2] << 8);
        switch (value) {
          case 0x5000: case 0x9000: case 0xb000: bankingCount[KONAMI_SCC]++; break;
          case 0x4000: case 0x8000: case 0xa000: bankingCount[KONAMI]++; break;
          case 0x6800: case 0x7800: bankingCount[ASC8]++; break;
          case 0x6000: bankingCount[KONAMI]++; bankingCount[ASC8]++; bankingCount[ASC16]++; break;
          case 0x7000: bankingCount[KONAMI_SCC]++; bankingCount[ASC8]++; bankingCount[ASC16]++; break;
          case 0x77ff: bankingCount[ASC16]++; break;
        }
      }
    }

    auto mapperNum = 0, max = 0;
    for(auto i : range(4)) {
      if (bankingCount[i] > max) {max = bankingCount[i]; mapperNum = i;}
    }

    switch(mapperNum) {
      case 0: board = "ASC16"; break;
      case 1: board = "ASC8"; break;
      case 2: board = "Konami"; break;
      case 3: board = "KonamiSCC"; break; 
    }
  }

  //Special Controllers
  //===================

  // Arkanoid (Japan)
  if (hash == "7100a087369bf03aa117f8103551047d888fc3eb86b339b1af1d51e028aee279") {
    vauspaddle = true;
  }

  string s;
  s += "game\n";
  s +={"  sha256: ", hash, "\n"};
  s +={"  name:   ", Medium::name(location), "\n"};
  s +={"  title:  ", Medium::name(location), "\n"};
  s += "  region: NTSC\n";  //database required to detect region
  s +={"  board: ", board, "\n"};
  if (vauspaddle) s += "  vauspaddle\n";
  s += "    memory\n";
  s += "      type: ROM\n";
  s +={"      size: 0x", hex(rom.size()), "\n"};
  s += "      content: Program\n";

  return s;
}


auto MSX::loadTape(string location) -> LoadResult {
  if(!inode::exists(location)) return romNotFound;
  if(location.iendsWith(".cas")) return loadCas(location);
  if(location.iendsWith(".tzx") || location.iendsWith(".tsx")) return loadTzx(location);

  this->location = location;
  this->manifest = analyzeTape(location);
  auto document = BML::unserialize(manifest);
  if(!document) return couldNotParseManifest;

  pak = std::make_shared<vfs::directory>();
  pak->setAttribute("title",      document["game/title"].string());
  pak->setAttribute("region",     document["game/region"].string());
  pak->setAttribute("range",      document["game/range"].natural());
  pak->setAttribute("frequency",  document["game/frequency"].natural());
  pak->setAttribute("length",     document["game/length"].natural());
  pak->setAttribute("writable",   true);
  pak->setAttribute("tape", true);
  pak->setAttribute("modified", false);
  pak->append("manifest.bml", manifest);
  if(directory::exists(location)) {
    pak->append("program.tape", vfs::disk::open({location, "program.tape"}, vfs::read));
  }
  if(file::exists(location)) {
    if(location.iendsWith(".wav")) {
        Decode::WAV wav;
        if (wav.open(location)) {
          std::vector<u64> samples;
          u64 lowest = ~0ull;
          u64 highest = 0;
          u64 range = pak->attribute("range").natural();

          for (int i = 0; i < wav.sample_length(); i++) {
            u64 sample = wav.read();
            if(wav.bitrate > 8) sample ^= 1ull << (wav.bitrate - 1);
            samples.push_back(sample);
            lowest = min(lowest, sample);
            highest = max(highest, sample);
          }

          std::vector<u8> data;
          for (auto sample : samples) {
            if(highest > lowest && range) {
              sample = ((sample - lowest) * range) / (highest - lowest);
            }

            for (int byte = 0; byte < sizeof(u64); byte++) {
              data.push_back((sample & (0xffull << (byte * 8))) >> (byte * 8));
            }
          }
        pak->append("program.tape", data);
      }
    }
  }

  return successful;
}

auto MSX::loadCas(string location) -> LoadResult {
  this->location = location;
  this->manifest = analyzeTape(location);
  auto document = BML::unserialize(manifest);
  if(!document) return couldNotParseManifest;

  auto input = file::read(location);
  if(input.empty()) return invalidROM;

  constexpr u32 frequency = 44100;
  constexpr u64 sampleRange = 255;
  constexpr u32 tstates = 3500000;
  constexpr u16 pilotPulseLength = 729;
  constexpr u16 zeroPulseLength = 1458;
  constexpr u16 onePulseLength = 729;
  constexpr u32 shortHeaderPulses = 30720;
  constexpr u32 longHeaderPulses = 122880;
  constexpr u32 gapMS = 1000;
  constexpr u8 header[8] = {0x1f, 0xa6, 0xde, 0xba, 0xcc, 0x13, 0x7d, 0x74};

  std::vector<u64> samples;
  bool level = false;

  auto appendPulse = [&](u32 length) {
    u32 count = max(1u, (u32)(0.5 + (double)frequency * (double)length / (double)tstates));
    for(u32 sample : range(count)) samples.push_back(level ? sampleRange : 0);
    level = !level;
  };

  auto appendSilence = [&](u32 milliseconds) {
    level = false;
    u32 count = (milliseconds * frequency) / 1000;
    for(u32 sample : range(count)) samples.push_back(0);
  };

  auto appendBit = [&](bool bit) {
    u32 pulses = bit ? 4 : 2;
    u32 length = bit ? onePulseLength : zeroPulseLength;
    for(u32 pulse : range(pulses)) appendPulse(length);
  };

  auto appendByte = [&](u8 byte) {
    appendBit(0);
    for(u32 bit : range(8)) appendBit(byte & (1 << bit));
    appendBit(1);
    appendBit(1);
  };

  auto appendHeader = [&](u32 pulses) {
    for(u32 pulse : range(pulses)) appendPulse(pilotPulseLength);
  };

  auto matchesHeader = [&](u32 offset) {
    if(offset + sizeof(header) > input.size()) return false;
    if(offset % sizeof(header) != 0) return false;
    for(u32 index : range(sizeof(header))) {
      if(input[offset + index] != header[index]) return false;
    }
    return true;
  };

  auto useLongHeader = [&](u32 offset) {
    if(offset + sizeof(header) + 10 > input.size()) return false;
    u8 value = input[offset + sizeof(header)];
    if(value != 0xd0 && value != 0xd3 && value != 0xea) return false;
    for(u32 index : range(10)) {
      if(input[offset + sizeof(header) + index] != value) return false;
    }
    return true;
  };

  for(u32 offset = 0; offset < input.size();) {
    if(matchesHeader(offset)) {
      bool longHeader = useLongHeader(offset);
      if(longHeader) appendSilence(gapMS);
      appendHeader(longHeader ? longHeaderPulses : shortHeaderPulses);
      offset += sizeof(header);
      continue;
    }

    appendByte(input[offset++]);
  }

  pak = std::make_shared<vfs::directory>();
  pak->setAttribute("title",      document["game/title"].string());
  pak->setAttribute("region",     document["game/region"].string());
  pak->setAttribute("range",      sampleRange);
  pak->setAttribute("frequency",  frequency);
  pak->setAttribute("length",     samples.size());
  pak->setAttribute("writable",   true);
  pak->setAttribute("tape", true);
  pak->setAttribute("modified", false);
  pak->append("manifest.bml", manifest);

  std::vector<u8> output;
  for(auto sample : samples) {
    for(int byte = 0; byte < sizeof(u64); byte++) {
      output.push_back((sample & (0xffull << (byte * 8))) >> (byte * 8));
    }
  }
  pak->append("program.tape", output);

  return successful;
}

auto MSX::loadTzx(string location) -> LoadResult {
  this->location = location;
  this->manifest = analyzeTape(location);
  auto document = BML::unserialize(manifest);
  if(!document) return couldNotParseManifest;

  auto input = file::read(location);
  TZXFile tzx;
  if(tzx.DecodeFile(input.data(), input.size()) == FileTypeUndetermined) return invalidROM;
  tzx.GenerateAudioData();

  pak = std::make_shared<vfs::directory>();
  pak->setAttribute("title",      document["game/title"].string());
  pak->setAttribute("region",     document["game/region"].string());
  pak->setAttribute("range",      (1 << 8) - 1);
  pak->setAttribute("frequency",  44100);
  pak->setAttribute("length",     tzx.GetAudioBufferLengthInSamples());
  pak->setAttribute("writable",   true);
  pak->setAttribute("tape", true);
  pak->setAttribute("modified", false);
  pak->append("manifest.bml", manifest);

  std::vector<u8> output;
  auto decodedData = tzx.GetAudioBufferPtr();
  for(int i = 0; i < tzx.GetAudioBufferLength();) {
    u64 sample = (u64)((i32)decodedData[i++] + 128);

    for (int byte = 0; byte < sizeof(u64); byte++) {
      output.push_back((sample & (0xffull << (byte * 8))) >> (byte * 8));
    }
  }
  pak->append("program.tape", output);

  return successful;
}

auto MSX::saveWav(string location) -> bool {
  std::vector<u16> data;
  auto fd = pak->read("program.tape");
  if(!fd) return false;
  u64 range = pak->attribute("range").natural();
  if(!range) return false;
  for(u32 i = 0; i < fd->size() / sizeof(u64); i++) {
    u64 sample = fd->readl(8);
    u16 normalized = (sample * 65535) / range;
    data.push_back(normalized ^ 0x8000);
  }
  fd.reset();
  return Encode::WAV::mono<u16>(
    location,
    data,
    pak->attribute("frequency").natural()
  );
}

auto MSX::analyzeTape(string location) -> string {
  string s;
  s += "game\n";
  s +={"  name:   ", Medium::name(location), "\n"};
  s +={"  title:  ", Medium::name(location), "\n"};
  s += "  region: NTSC\n";  //database required to detect region

  if(location.iendsWith(".wav")) {
    Decode::WAV wav;
    if (wav.open(location)) {
      s +={"  range:     ", ((1ull << wav.bitrate) - 1), "\n"};
      s +={"  frequency: ", wav.frequency, "\n"};
      s +={"  length:    ", wav.sample_length(), "\n"};
      wav.close();
    }
  } else if(location.iendsWith(".tzx") || location.iendsWith(".tsx") || location.iendsWith(".cas")) {
    s += "  range:     255\n";
    s += "  frequency: 44100\n";
    s += "  length:    0\n";
  }

  return s;
}
