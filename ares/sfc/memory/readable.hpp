struct ReadableMemory : AbstractMemory {
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

  auto read(n24 address, n8 data = 0) -> n8 override {
    return self.data[address];
  }

  auto write(n24 address, n8 data) -> void override {
  }

  auto program(n24 address, n8 data) -> void {
    self.data[address] = data;
  }

  auto operator[](n24 address) const -> n8 {
    return self.data[address];
  }

  auto serialize(serializer& s) -> void {
    s(array_span<n8>{self.data, self.size});
  }

private:
  struct {
    n8* data = nullptr;
    u32 size = 0;
  } self;
};
