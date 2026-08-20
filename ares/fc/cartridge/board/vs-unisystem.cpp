struct VsUniSystem : Interface {
  static auto create(string id) -> Interface* {
    if(id == "nintendo/vs") return new VsUniSystem;
    return nullptr;
  }

private:
  Memory::Readable<n8> programROM;
  Memory::Readable<n8> characterROM;
  Memory::Writable<n8> characterRAM;
  Memory::Writable<n8> workRAM;
  Memory::Writable<n8> nametableRAM;
  std::unique_ptr<Vs::Interface> board;
  std::unique_ptr<Vs::Protection> protection;
  n8 coinLatch;

public:
  auto load() -> void override {
    Interface::load(programROM, "program.rom");
    Interface::load(characterROM, "character.rom");
    if(!characterROM) characterRAM.allocate(0x2000);
    workRAM.allocate(0x0800, 0x00);
    nametableRAM.allocate(0x1000);

    auto characterSize = characterROM ? characterROM.size() : characterRAM.size();
    board.reset(Vs::Interface::create(pak->attribute("mapper"), programROM.size(), characterSize));
    protection.reset(Vs::Protection::create(pak->attribute("protection")));
  }

  auto readPRG(n32 address, n8 data) -> n8 override {
    if(address >= 0x4020 && address <= 0x5fff) return protection->readPRG(address, coinLatch);
    if(address >= 0x6000 && address <= 0x7fff) return workRAM.read(address & 0x07ff);
    if(address >= 0x8000) {
      if(auto mapped = board->addressPRG(address)) return programROM.read(*mapped);
    }
    return data;
  }

  auto writePRG(n32 address, n8 data) -> void override {
    if(address == 0x4016) return board->writePRG(address, data);
    if(address >= 0x4020 && address <= 0x5fff) {
      coinLatch = data;
      return;
    }
    if(address >= 0x6000 && address <= 0x7fff) return workRAM.write(address & 0x07ff, data);
    if(address >= 0x8000) return board->writePRG(address, data);
  }

  auto readCHR(n32 address, n8 data) -> n8 override {
    if(address & 0x2000) return nametableRAM.read(address & 0x0fff);
    if(auto mapped = board->addressCHR(address)) {
      if(characterROM) return characterROM.read(*mapped);
      if(characterRAM) return characterRAM.read(*mapped);
    }
    return data;
  }

  auto writeCHR(n32 address, n8 data) -> void override {
    if(address & 0x2000) return nametableRAM.write(address & 0x0fff, data);
    if(!characterRAM) return;
    if(auto mapped = board->addressCHR(address)) return characterRAM.write(*mapped, data);
  }

  auto power() -> void override {
    coinLatch = 0;
    board->power();
    protection->power();
  }

  auto serialize(serializer& s) -> void override {
    s(characterRAM);
    s(workRAM);
    s(nametableRAM);
    s(coinLatch);
    board->serialize(s);
    protection->serialize(s);
  }
};
