struct Mega32X : Interface {
  using Interface::Interface;

  auto load() -> void override {
    if(auto fp = pak->read("program.rom")) {
      m32x.rom.allocate(fp->size() >> 1);
      for(auto address : range(fp->size() >> 1)) m32x.rom.program(address, fp->readm(2L));
    }
  }

  auto save() -> void override {
  }

  auto read(n1 upper, n1 lower, n22 address, n16 data) -> n16 override {
    return m32x.readExternal(upper, lower, address, data);
  }

  auto write(n1 upper, n1 lower, n22 address, n16 data) -> void override {
    return m32x.writeExternal(upper, lower, address, data);
  }

  auto readIO(n1 upper, n1 lower, n24 address, n16 data) -> n16 override {
    return data = m32x.readExternalIO(upper, lower, address, data);
  }

  auto writeIO(n1 upper, n1 lower, n24 address, n16 data) -> void override {
    return m32x.writeExternalIO(upper, lower, address, data);
  }

  auto vblank(bool line) -> void override {
    return m32x.vblank(line);
  }

  auto hblank(bool line) -> void override {
    return m32x.hblank(line);
  }

  auto power(bool reset) -> void override {
  }

  auto serialize(serializer& s) -> void override {
  }
};
