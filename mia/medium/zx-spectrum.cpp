struct ZXSpectrum : Medium {
  auto name() -> string override { return "ZX Spectrum"; }
  auto extensions() -> std::vector<string> override { return {"wav", "tzx", "tap" }; }
  auto load(string location) -> LoadResult override;
  auto loadWav(string location) -> LoadResult;
  auto loadTzx(string location) -> LoadResult;
  auto save(string location) -> bool override;
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

  pak = new vfs::directory;
  pak->setAttribute("title",     document["game/title"].string());
  pak->setAttribute("range",     (1 << 8) - 1);
  pak->setAttribute("frequency", 44100);
  pak->setAttribute("length",    tzx.GetAudioBufferLengthInSamples());
  pak->append("manifest.bml",    manifest);

  std::vector<u8> output;
  auto decodedData = tzx.GetAudioBufferPtr();
  for(int i = 0; i < tzx.GetAudioBufferLength();) {
    u64 sample = (u64)((i32)decodedData[i++] + 128);

    for (int byte = 0; byte < sizeof(u64); byte++) {
      output.push_back((sample & (0xff << (byte * 8))) >> (byte * 8));
    }
  }
  pak->append("program.tape", output);

  return successful;
}

auto ZXSpectrum::loadWav(string location) -> LoadResult {
  this->location = location;
  this->manifest = analyze(location);
  auto document = BML::unserialize(manifest);
  if(!document) return couldNotParseManifest;

  pak = new vfs::directory;
  pak->setAttribute("title",      document["game/title"].string());
  pak->setAttribute("range",      document["game/range"].natural());
  pak->setAttribute("frequency",  document["game/frequency"].natural());
  pak->setAttribute("length",     document["game/length"].natural());
  pak->append("manifest.bml", manifest);
  if(directory::exists(location)) {
    pak->append("program.tape", vfs::disk::open({location, "program.tape"}, vfs::read));
  }
  if(file::exists(location)) {
    if(location.iendsWith(".wav")) {
      Decode::WAV wav;
      if (wav.open(location)) {
        std::vector<u8> data;
        for (int i = 0; i < wav.size(); i++) {
          u64 sample = wav.read();

          for (int byte = 0; byte < sizeof(u64); byte++) {
            data.push_back((sample & (0xff << (byte * 8))) >> (byte * 8));
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
  return true;
}

auto ZXSpectrum::analyze(string location) -> string {
  string s;
  s += "game\n";
  s +={"  name:   ", Medium::name(location), "\n"};
  s +={"  title:  ", Medium::name(location), "\n"};

  if(location.iendsWith(".wav")) {
    Decode::WAV wav;
    if (wav.open(location)) {
      s +={"  range:     ", ((1 << wav.bitrate) - 1), "\n"};
      s +={"  frequency: ", wav.frequency, "\n"};
      s +={"  length:    ", wav.size(), "\n"};
      wav.close();
    }
  }

  return s;
}
