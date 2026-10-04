struct PersistentMemory {
  std::vector<u8> memory;
  string name;
  u8 fill = 0x00;
  bool dirty = false;

  auto load(VFS::Pak pak, string filename, u32 size, u8 erased = 0x00) -> void {
    name = filename;
    fill = erased;
    dirty = false;
    memory.assign(size, fill);
    if(!pak) return;
    if(auto fp = pak->read(name)) {
      if(fp->size() != size) {
        dirty = true;
        return;
      }
      fp->read({memory.data(), memory.size()});
    }
  }

  auto read(u32 address) const -> u8 {
    if(address >= memory.size()) return fill;
    return memory[address];
  }

  auto write(u32 address, u8 data) -> bool {
    if(address >= memory.size()) return false;
    if(memory[address] == data) return true;
    memory[address] = data;
    dirty = true;
    return true;
  }

  auto replace(std::span<const u8> source) -> bool {
    if(source.size() != memory.size()) return false;
    if(std::equal(source.begin(), source.end(), memory.begin())) return true;
    std::copy(source.begin(), source.end(), memory.begin());
    dirty = true;
    return true;
  }

  auto erase() -> void {
    for(auto& byte : memory) {
      if(byte != fill) dirty = true;
      byte = fill;
    }
  }

  auto flush(VFS::Pak pak) -> bool {
    if(!dirty) return true;
    if(!pak) return false;
    if(!pak->read(name)) pak->append(name, memory.size());
    auto fp = pak->write(name);
    if(!fp) return false;
    fp->resize(memory.size());
    fp->write({memory.data(), memory.size()});
    dirty = false;
    return true;
  }
};
