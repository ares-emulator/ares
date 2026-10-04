auto KidVidAudio::load(VFS::Pak pak) -> void {
  static constexpr u64 MaximumFileSize = 64_MiB;
  static constexpr std::array<const char*, (u32)File::Count> Names = {
    "KVS3.WAV", "KVS1.WAV", "KVS2.WAV",
    "KVB3.WAV", "KVB1.WAV", "KVB2.WAV",
    "KVSHARED.WAV",
  };

  reset();
  sources = {};
  mediaIdentity = 14695981039346656037ull;
  for(u32 index : range((u32)File::Count)) {
    auto fp = pak ? pak->read(Names[index]) : VFS::File{};
    if(!fp) continue;
    if(fp->size() > MaximumFileSize) continue;
    auto& item = sources[index];
    item.bytes.resize(fp->size());
    if(!item.bytes.empty()) memory::copy(item.bytes.data(), fp->data(), item.bytes.size());
    for(auto byte : item.bytes) {
      mediaIdentity ^= byte;
      mediaIdentity *= 1099511628211ull;
    }
    mediaIdentity ^= index;
    mediaIdentity *= 1099511628211ull;
    item.valid = parse(item);
  }
}

auto KidVidAudio::reset() -> void {
  activeFile = File::Shared;
  cursor = 0;
  end = 0;
  active = false;
}

auto KidVidAudio::available(File file) const -> bool {
  return file < File::Count && source(file).valid;
}

auto KidVidAudio::play(File file, u32 begin, u32 end) -> bool {
  stop();
  if(!available(file)) return false;
  auto& item = source(file);
  if(begin < item.dataBegin || end < begin || end > item.dataEnd) return false;
  activeFile = file;
  cursor = begin;
  this->end = end;
  active = cursor < end;
  return active;
}

auto KidVidAudio::stop() -> void {
  active = false;
  cursor = end;
}

auto KidVidAudio::remaining() const -> u32 {
  if(!active || cursor >= end) return 0;
  return end - cursor;
}

auto KidVidAudio::clock() -> f64 {
  if(!active) return 0.0;
  auto& item = source(activeFile);
  if(cursor >= end || cursor >= item.bytes.size()) {
    stop();
    return 0.0;
  }
  auto sample = item.bytes[cursor++];
  if(cursor >= end) active = false;
  return ((s32)sample - 128) / 128.0;
}

auto KidVidAudio::serialize(serializer& s) -> bool {
  auto identity = mediaIdentity;
  u32 file = (u32)activeFile;
  s(identity);
  s(file);
  s(cursor);
  s(end);
  s(active);

  if(s.reading()) {
    if(identity != mediaIdentity || file >= (u32)File::Count) {
      reset();
      return false;
    }
    activeFile = (File)file;
    if(!active) {
      reset();
      return true;
    }
    auto& item = source(activeFile);
    if(!item.valid || cursor < item.dataBegin || end < cursor || end > item.dataEnd) {
      reset();
      return false;
    }
  }
  return true;
}

auto KidVidAudio::parse(Source& source) -> bool {
  auto& bytes = source.bytes;
  if(bytes.size() < 12) return false;
  auto tag = [&](u32 offset, const char* text) {
    return offset <= bytes.size() - 4 && bytes[offset + 0] == text[0] && bytes[offset + 1] == text[1]
      && bytes[offset + 2] == text[2] && bytes[offset + 3] == text[3];
  };
  auto read16 = [&](u32 offset) -> u16 {
    return (u16)bytes[offset] | (u16)bytes[offset + 1] << 8;
  };
  auto read32 = [&](u32 offset) -> u32 {
    return (u32)bytes[offset] | (u32)bytes[offset + 1] << 8
      | (u32)bytes[offset + 2] << 16 | (u32)bytes[offset + 3] << 24;
  };

  if(!tag(0, "RIFF") || !tag(8, "WAVE")) return false;
  auto declaredEnd = (u64)read32(4) + 8;
  if(declaredEnd > bytes.size() || declaredEnd < 12) return false;

  bool formatValid = false;
  bool dataFound = false;
  for(u64 offset = 12; offset + 8 <= declaredEnd;) {
    auto chunkSize = read32((u32)offset + 4);
    auto dataBegin = offset + 8;
    auto dataEnd = dataBegin + chunkSize;
    if(dataEnd > declaredEnd || dataEnd < dataBegin) return false;

    if(tag((u32)offset, "fmt ")) {
      if(chunkSize < 16) return false;
      formatValid = read16((u32)dataBegin + 0) == 1
        && read16((u32)dataBegin + 2) == 1
        && read32((u32)dataBegin + 4) == 16000
        && read16((u32)dataBegin + 12) == 1
        && read16((u32)dataBegin + 14) == 8;
    }
    if(tag((u32)offset, "data") && !dataFound) {
      source.dataBegin = dataBegin;
      source.dataEnd = dataEnd;
      dataFound = true;
    }

    offset = dataEnd;
    //Several canonical KidVid WAVs end on an odd-sized data chunk without a
    //physical pad byte. A pad is required before another chunk, not after EOF.
    if((chunkSize & 1) && offset < declaredEnd) offset++;
    if(offset > declaredEnd) return false;
  }
  return formatValid && dataFound && source.dataBegin < source.dataEnd;
}
