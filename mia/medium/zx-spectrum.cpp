struct ZXSpectrum : Medium {
  auto name() -> string override { return "ZX Spectrum"; }
  auto extensions() -> vector<string> override { return {"wav" }; }
  auto load(string location) -> bool override;
  auto save(string location) -> bool override;
  auto analyze(string location) -> string;
};

auto ZXSpectrum::load(string location) -> bool {
  if(!inode::exists(location)) return false;

  this->location = location;
  this->manifest = analyze(location);
  auto document = BML::unserialize(manifest);
  if(!document) return false;

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
        vector <u8> data;
        for (int i = 0; i < wav.size(); i++) {
          u64 sample = wav.read();

          for (int byte = 0; byte < sizeof(u64); byte++) {
            data.append((sample & (0xff << (byte * 8))) >> (byte * 8));
          }
        }
        pak->append("program.tape", data);
      }
    }
  }

  return true;
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
