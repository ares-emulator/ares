struct Atari32In1 : Interface {
  using Interface::Interface;
  Memory::Readable<n8> rom;
  n5 game;

  auto load() -> void override {
    Interface::load(rom, "program.rom");
  }

  auto read(n16 address, n8 data) -> n8 override {
    if(address.bit(12)) return rom.read(game * 0x800 + (address & 0x07ff));
    return data;
  }

  auto write(n16 address, n8 data) -> n8 override {
    return data;
  }

  auto power(bool reset) -> void override {
    //MAME's verified software-list contract cycles 32 2 KiB games on machine reset.
    //Stella instead presents the same image as frontend-selected slices; no PCB or schematic
    //presently proves the physical counter or latch behavior.
    if(reset) game++;
    else game = 0;
  }

  auto serialize(serializer& s) -> void override {
    s(game);
  }
};
