struct KidVidAudio {
  enum class File : u32 {
    Smurfs3,
    Smurfs1,
    Smurfs2,
    Bears3,
    Bears1,
    Bears2,
    Shared,
    Count,
  };

  auto load(VFS::Pak) -> void;
  auto reset() -> void;
  auto available(File) const -> bool;
  auto play(File, u32 begin, u32 end) -> bool;
  auto stop() -> void;
  auto playing() const -> bool { return active; }
  auto remaining() const -> u32;
  auto clock() -> f64;
  auto serialize(serializer&) -> bool;

private:
  struct Source {
    std::vector<u8> bytes;
    u32 dataBegin = 0;
    u32 dataEnd = 0;
    bool valid = false;
  };

  auto parse(Source&) -> bool;
  auto source(File file) -> Source& { return sources[(u32)file]; }
  auto source(File file) const -> const Source& { return sources[(u32)file]; }

  std::array<Source, (u32)File::Count> sources;
  u64 mediaIdentity = 0;
  File activeFile = File::Shared;
  u32 cursor = 0;
  u32 end = 0;
  bool active = false;
};
