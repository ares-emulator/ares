struct SC3000 : SG1000 {
  auto name() -> string override { return "SC-3000"; }
  auto extensions() -> std::vector<string> override { return {"sg", "sc"}; }
  auto load(string location) -> LoadResult override;
  auto save(string location) -> bool override;
  auto loadTape(string location) -> LoadResult;
  auto loadBit(string location) -> LoadResult;
  auto saveWav(string location) -> bool;
  auto analyzeTape(string location) -> string;
};

auto SC3000::load(string location) -> LoadResult {
  if(location.iendsWith(".wav") || location.iendsWith(".bit")) {
    return loadTape(location);
  }

  return SG1000::load(location);
}

auto SC3000::save(string location) -> bool {
  if(pak && pak->attribute("tape").boolean()) {
    if(!pak->attribute("modified").boolean()) return true;

    if(file::exists(location) && location.iendsWith(".wav")) {
      return saveWav(location);
    }

    if(file::exists(location) && location.iendsWith(".bit")) {
      return saveWav(saveLocation(location, "program.tape", ".wav"));
    }
  }

  return SG1000::save(location);
}

auto SC3000::loadTape(string location) -> LoadResult {
  if(!inode::exists(location)) return romNotFound;
  if(location.iendsWith(".bit")) return loadBit(location);

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
  if(file::exists(location) && location.iendsWith(".wav")) {
    Decode::WAV wav;
    if(wav.open(location)) {
      std::vector<u64> samples;
      u64 lowest = ~0ull;
      u64 highest = 0;
      u64 range = pak->attribute("range").natural();

      for(int i = 0; i < wav.sample_length(); i++) {
        u64 sample = wav.read();
        if(wav.bitrate > 8) sample ^= 1ull << (wav.bitrate - 1);
        samples.push_back(sample);
        lowest = min(lowest, sample);
        highest = max(highest, sample);
      }

      std::vector<u8> data;
      for(auto sample : samples) {
        if(highest > lowest && range) {
          sample = ((sample - lowest) * range) / (highest - lowest);
        }

        for(int byte = 0; byte < sizeof(u64); byte++) {
          data.push_back((sample & (0xffull << (byte * 8))) >> (byte * 8));
        }
      }
      pak->append("program.tape", data);
    }
  }

  return successful;
}

auto SC3000::loadBit(string location) -> LoadResult {
  this->location = location;
  this->manifest = analyzeTape(location);
  auto document = BML::unserialize(manifest);
  if(!document) return couldNotParseManifest;

  auto input = file::read(location);
  if(input.empty()) return invalidROM;

  constexpr u32 frequency = 44100;
  constexpr u64 sampleRange = 255;

  std::vector<u64> samples;
  bool level = false;

  auto appendSamples = [&](u32 count, u64 sample) {
    for(u32 index : range(count)) samples.push_back(sample);
  };

  auto appendSilence = [&] {
    appendSamples((u32)(0.5 + (double)frequency / 1200.0), 0);
    level = false;
  };

  auto appendHalfCycle = [&](u32 hz) {
    appendSamples(max(1u, (u32)(0.5 + (double)frequency / (double)(hz * 2))), level ? sampleRange : 0);
    level = !level;
  };

  auto appendCycle = [&](u32 hz) {
    appendHalfCycle(hz);
    appendHalfCycle(hz);
  };

  for(auto byte : input) {
    if(byte == ' ') {
      appendSilence();
    } else if(byte == '0') {
      appendCycle(1200);
    } else if(byte == '1') {
      appendCycle(2400);
      appendCycle(2400);
    } else {
      return invalidROM;
    }
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

auto SC3000::saveWav(string location) -> bool {
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

auto SC3000::analyzeTape(string location) -> string {
  string s;
  s += "game\n";
  s +={"  name:   ", Medium::name(location), "\n"};
  s +={"  title:  ", Medium::name(location), "\n"};
  s += "  region: NTSC, PAL\n";

  if(location.iendsWith(".wav")) {
    Decode::WAV wav;
    if(wav.open(location)) {
      s +={"  range:     ", ((1ull << wav.bitrate) - 1), "\n"};
      s +={"  frequency: ", wav.frequency, "\n"};
      s +={"  length:    ", wav.sample_length(), "\n"};
      wav.close();
    }
  } else if(location.iendsWith(".bit")) {
    s += "  range:     255\n";
    s += "  frequency: 44100\n";
    s += "  length:    0\n";
  }

  return s;
}

struct SC3000Tape : SC3000 {
  auto name() -> string override { return "SC-3000 Tape"; }
  auto saveName() -> string override { return "SC-3000"; }
  auto extensions() -> std::vector<string> override { return {"wav", "bit"}; }

  auto load(string location) -> LoadResult override {
    return loadTape(location);
  }
};
