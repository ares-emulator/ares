struct MagicFloor : Interface {
  static auto create(string id) -> Interface* {
    if(id == "MAGICFLOOR") return new MagicFloor();
    return nullptr;
  }

  Memory::Readable<n8> programROM;

  auto load() -> void override {
    Interface::load(programROM, "program.rom");
    pin = (n2)(pak->attribute("pinout/va10").natural() - 10);
  }

  auto readPRG(n32 address, n8 data) -> n8 override {
    if(address < 0x8000) return data; 
    return programROM.read((n15)address);
  }

  auto readCHR(n32 address, n8 data) -> n8 override {
    return ppu.readCIRAM(((address >> pin) & 0x400) | (n10)address);
  }

  auto writeCHR(n32 address, n8 data) -> void override {
    return ppu.writeCIRAM(((address >> pin) & 0x400) | (n10)address, data);
  }

  auto serialize(serializer& s) -> void override {
    s(pin);
  }

  n2 pin;
};
