struct Linear32K : Interface {
  using Interface::Interface;

  auto load() -> bool override {
    auto fp = pak->read("program.rom");
    if(!fp || fp->size() != 32_KiB) return false;
    rom.allocate(32_KiB);
    rom.load(fp);
    return true;
  }

  auto unload() -> void override {
    rom.reset();
  }

  auto read(n16 address, n8 data) -> n8 override {
    return peek(address, data);
  }

  auto peek(n16 address, n8 data) const -> n8 override {
    if(!rom) return data;
    return rom.read(address & 0x7fff);
  }

private:
  Memory::Readable<n8> rom;
};
