//Datach's serial-memory bus; barcode input is not implemented.
struct BandaiDatach : BandaiLZ93D50 {
  static auto create(string id) -> Interface* {
    if(id == "BANDAI-DATACH") return new BandaiDatach;
    return nullptr;
  }

  Bandai24C01 external;
  bool hasExternal = false;

  auto load() -> void override {
    BandaiLZ93D50::load();
    if(auto fp = pak->read("external.eeprom")) {
      if(hasExternal = fp->size() == 128) fp->read(external.memory, 128);
    }
  }

  auto save() -> void override {
    BandaiLZ93D50::save();
    if(hasExternal) {
      if(auto fp = pak->write("external.eeprom")) fp->write(external.memory, 128);
    }
  }

  auto readPRG(n32 address, n8 data) -> n8 override {
    data = BandaiLZ93D50::readPRG(address, data);
    if(address >= 0x6000 && address < 0x8000) {
      data.bit(3) = 0;  //idle barcode signal
      if(hasExternal) data.bit(4) = data.bit(4) && external.read();
    }
    return data;
  }

  auto writePRG(n32 address, n8 data) -> void override {
    if(address < 0x8000) return;
    auto reg = address & 15;
    if(reg <= 7) {
      if(reg <= 3 && hasExternal) external.write(data.bit(3), external.data);
      return;
    }
    BandaiLZ93D50::writePRG(address, data);
    if(reg == 13 && hasExternal) external.write(external.clock, data.bit(6));
  }

  auto power() -> void override {
    BandaiLZ93D50::power();
    external.power();
  }

  auto serialize(serializer& s) -> void override {
    BandaiLZ93D50::serialize(s);
    if(hasExternal) s(external);
  }
};
