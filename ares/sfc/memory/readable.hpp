struct ReadableMemory : AbstractMemory {
  auto reset() -> void override {
    delete[] self.data;
    self.data = nullptr;
    self.size = 0;
  }

  auto allocate(uint size, uint8 fill = 0xff) -> void override {
    delete[] self.data;
    self.data = new uint8[self.size = size];
    for(uint address : range(size)) self.data[address] = fill;
  }

  auto load(shared_pointer<vfs::file> fp) -> void {
    fp->read(self.data, min(fp->size(), self.size));
  }

  auto save(shared_pointer<vfs::file> fp) -> void {
    fp->write(self.data, self.size);
  }

  auto data() -> uint8* override {
    return self.data;
  }

  auto size() const -> uint override {
    return self.size;
  }

  auto read(uint24 address, uint8 data = 0) -> uint8 override {
    return self.data[address];
  }

  auto write(uint24 address, uint8 data) -> void override {
  }

  auto program(uint24 address, uint8 data) -> void {
    self.data[address] = data;
  }

  auto operator[](uint24 address) const -> uint8 {
    return self.data[address];
  }

  auto serialize(serializer& s) -> void {
    s(array_span<uint8>{self.data, self.size});
  }

private:
  struct {
    uint8* data = nullptr;
    uint size = 0;
  } self;
};
