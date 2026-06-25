struct ZXSpectrum : Medium {
  auto name() -> string override { return "ZX Spectrum"; }
  auto extensions() -> std::vector<string> override { return {"wav", "tzx", "tap" }; }
  auto load(string location) -> LoadResult override;
  auto loadWav(string location, string originalLocation = {}) -> LoadResult;
  auto loadTzx(string location) -> LoadResult;
  auto save(string location) -> bool override;
  auto saveWav(string location) -> bool;
  auto analyze(string location) -> string;
};

auto ZXSpectrum::load(string location) -> LoadResult {
  if(!inode::exists(location)) return romNotFound;

  if(location.iendsWith(".tap") || location.iendsWith(".tzx")) return loadTzx(location);
  if(location.iendsWith(".wav")) return loadWav(location);
  return invalidROM;
}

auto ZXSpectrum::loadTzx(string location) -> LoadResult {
  this->location = location;
  this->manifest = analyze(location);
  auto document = BML::unserialize(manifest);
  if(!document) return couldNotParseManifest;

  auto input = file::read(location);
  TZXFile tzx;
  if(tzx.DecodeFile(input.data(), input.size()) == FileTypeUndetermined) return invalidROM;
  tzx.GenerateAudioData();

  pak = std::make_shared<vfs::directory>();
  pak->setAttribute("title",     document["game/title"].string());
  pak->setAttribute("range",     (1 << 8) - 1);
  pak->setAttribute("frequency", 44100);
  pak->setAttribute("length",    tzx.GetAudioBufferLengthInSamples());
  pak->setAttribute("writable",  true);
  pak->setAttribute("modified",  false);
  pak->append("manifest.bml",    manifest);

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

auto ZXSpectrum::loadWav(string location, string originalLocation) -> LoadResult {
  this->location = originalLocation ? originalLocation : location;
  this->manifest = analyze(location);
  auto document = BML::unserialize(manifest);
  if(!document) return couldNotParseManifest;

  pak = std::make_shared<vfs::directory>();
  pak->setAttribute("title",      Medium::name(this->location));
  pak->setAttribute("range",      document["game/range"].natural());
  pak->setAttribute("frequency",  document["game/frequency"].natural());
  pak->setAttribute("length",     document["game/length"].natural());
  pak->setAttribute("writable",   true);
  pak->setAttribute("modified",   false);
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

auto ZXSpectrum::save(string location) -> bool {
  auto document = BML::unserialize(manifest);
  if(!pak) return false;
  if(!pak->attribute("modified").boolean()) return true;

  if(directory::exists(location)) {
    return Pak::save("program.tape", ".sav");
  }

  if(file::exists(location) && location.iendsWith(".wav")) {
    return saveWav(location);
  }

  if(file::exists(location) && (location.iendsWith(".tap") || location.iendsWith(".tzx"))) {
    return saveWav(saveLocation(location, "program.tape", ".wav"));
  }

  return true;
}

auto ZXSpectrum::saveWav(string location) -> bool {
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

auto ZXSpectrum::analyze(string location) -> string {
  string s;
  s += "game\n";
  s +={"  name:   ", Medium::name(location), "\n"};
  s +={"  title:  ", Medium::name(location), "\n"};

  if(location.iendsWith(".wav")) {
    Decode::WAV wav;
    if (wav.open(location)) {
      s +={"  range:     ", ((1ull << wav.bitrate) - 1), "\n"};
      s +={"  frequency: ", wav.frequency, "\n"};
      s +={"  length:    ", wav.sample_length(), "\n"};
      wav.close();
    }
  }

  return s;
}
