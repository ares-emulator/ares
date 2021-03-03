struct ProtectableMemory : AbstractMemory {
  auto reset() -> void override {
    delete[] self.data;
    self.data = nullptr;
    self.size = 0;
  }

  auto allocate(u32 size, n8 fill = 0xff) -> void override {
    delete[] self.data;
    self.data = new n8[self.size = size];
    for(u32 address : range(size)) self.data[address] = fill;
  }

  auto load(VFS::File fp) -> void override {
    if(!self.size) allocate(fp->size());
    fp->read({self.data, min(fp->size(), self.size)});
  }

  auto save(VFS::File fp) -> void override {
    fp->write({self.data, min(fp->size(), self.size)});
  }

  auto data() -> n8* override {
    return self.data;
  }

  auto size() const -> u32 override {
    return self.size;
  }

  auto writable() const -> bool {
    return self.writable;
  }

  auto writable(bool writable) -> void {
    self.writable = writable;
  }

  auto read(n24 address, n8 data = 0) -> n8 override {
    return self.data[address];
  }

  auto write(n24 address, n8 data) -> void override {
    if(!self.writable) return;
    self.data[address] = data;
  }

  auto operator[](n24 address) const -> n8 {
    return self.data[address];
  }

private:
  struct {
    n8*  data = nullptr;
    u32  size = 0;
    bool writable = false;
  } self;
};
