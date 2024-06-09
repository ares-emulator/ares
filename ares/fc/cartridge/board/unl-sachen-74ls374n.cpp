/// 74LS374N                          SA-020                            SA-015
///              .---V---.                         .---V---.                         .---V---.
///      R5.0 <- | 01 20 | -- Vcc       PRG A15 <- | 01 20 | -- Vcc       PRG A15 <- | 01 20 | -- Vcc
/// CIRAM A10 <- | 02 19 | <- CPU A8  CIRAM A10 <- | 02 19 | <- CPU A8  CIRAM A10 <- | 02 19 | <- CPU A8
///   PPU A11 -> | 03 18 | <- R/W       PPU A11 -> | 03 18 | <- R/W       PPU A11 -> | 03 18 | <- R/W
///      R5.1 <- | 04 17 | <- M2        PRG A16 <- | 04 17 | <- M2        PRG A16 <- | 04 17 | <- M2
///   CPU A14 -> | 05 16 | <- PPU A10   CPU A14 -> | 05 16 | <- PPU A10   CPU A14 -> | 05 16 | <- PPU A10
///   CPU  A0 -> | 06 15 | <- /ROMSEL   CPU  A0 -> | 06 15 | <- /ROMSEL   CPU  A0 -> | 06 15 | <- /ROMSEL
///   CPU  D0 <> | 07 14 | <> CPU D2    CPU  D0 <> | 07 14 | <> CPU D2    CPU  D0 <> | 07 14 | <> CPU D2 or Vcc, depending on solder pad setting
///   CPU  D1 <> | 08 13 | -> R6.1      CPU  D1 <> | 08 13 | -> CHR A14   CPU  D1 <> | 08 13 | -> CHR A14
///      R2.0 <- | 09 12 | -> R6.0      CHR A16 <- | 09 12 | -> CHR A13       N/C <- | 09 12 | -> CHR A13
///       GND -- | 10 11 | -> R4.0          GND -- | 10 11 | -> CHR A15       GND -- | 10 11 | -> CHR A15
///              '-------'                         '-------'                         '-------'
///
/// SA-020A:
///   CHR A16 A15 A14 A13: R2.0 R4.0 R6.1 R6.0
///   PRG A16 A15: R5.1 R5.0
///
/// SA-015:
///   CHR A15 A14 A13: R4.0 R6.1 R6.0
///   PRG A16 A15: R5.1 R5.0
struct Sachen74LS374N : Interface {
  static auto create(string id) -> Interface* {
    if (id == "UNL-Sachen-74LS374N") return new Sachen74LS374N(Revision::SACHEN_74LS374N);
    if (id == "UNL-Sachen-74LS374NA") return new Sachen74LS374N(Revision::SACHEN_74LS374NA);
    return nullptr;
  }

  Memory::Readable<n8> programROM;
  Memory::Readable<n8> characterROM;

  // The circuit board has a solder pad that selects
  // whether ASIC pin 14 is connected to CPU D2 or to
  // Vcc. In the latter setting, the ASIC sees all
  // writes as being OR'd with $04, while on reads,
  // D2 is open bus. The pad setting only makes a
  // difference for the game Shōgi Gakuen, which
  // writes to R2 and reads from R6 to check whether
  // it now holds that written value, as would be
  // the case if the solder pad were in the Vcc
  // position; depending on the setting, a different
  // copyright tag ("Sachen" vs. "Sachen and Hacker
  // International") is displayed.
  Node::Setting::Boolean solderPad;

  enum class Revision : u32 {
    SACHEN_74LS374N,
    SACHEN_74LS374NA,
  } revision;

  Sachen74LS374N(Revision revision) : revision(revision) {}

  auto load() -> void override {
    Interface::load(programROM, "program.rom");
    Interface::load(characterROM, "character.rom");

    if (revision == Revision::SACHEN_74LS374N) {
      solderPad = cartridge.node->append<Node::Setting::Boolean>("Solder Pad", false);
    }
  }

  auto unload() -> void override {
    if (revision == Revision::SACHEN_74LS374N) {
      solderPad.reset();
    }
  }

  auto readPRG(n32 address, n8 data) -> n8 override {
    if (address < 0x4100) return data;
    if (address < 0x8000) {
      if ((address & 0x4101) == 0x4101)
        if (revision == Revision::SACHEN_74LS374N && solderPad->value())
          return (data & ~0x07) | regs[select].bit(0, 1) | 0x04;
        else
          return (data & ~0x07) | regs[select].bit(0, 2);
      return data;
    }
    address = prgBank << 15 | (n15)address;
    data = programROM.read(address);
    return data;
  }

  auto writePRG(n32 address, n8 data) -> void override {
    if (address < 0x4100) return;
    if (address < 0x8000) {
      address &= 0x4101;
      if (address == 0x4100) {
        select = data.bit(0, 2);
      } else {
        if (revision == Revision::SACHEN_74LS374N && solderPad->value()) {
          regs[select] = data.bit(0, 1) | 0x04;
        } else {
          regs[select] = data.bit(0, 2);
        }
        regs[select] = data.bit(0, 2);
        update();
        return;
      }
    }
  }

  auto readCHR(n32 address, n8 data) -> n8 override {
    if (address & 0x2000) return ppu.readCIRAM(addressCIRAM(address));
    return characterROM.read(chrBank << 13 | (n13)address);
  }

  auto writeCHR(n32 address, n8 data) -> void override {
    if (address & 0x2000) return ppu.writeCIRAM(addressCIRAM(address), data);
  }

  auto power() -> void override {
    for (n8 &reg : regs) reg = 0;

    update();
  }

  auto serialize(serializer& s) -> void override {
    s(regs);
    s(select);

    if (s.reading()) update();
  }

  auto update() -> void {
    if (revision == Revision::SACHEN_74LS374N) {
      chrBank = regs[4].bit(0) << 2 | regs[6].bit(0, 1);
    } else {
      chrBank = regs[2].bit(0) << 3 | regs[4].bit(0) << 2 | regs[6].bit(0, 1);
    }
    prgBank = regs[5].bit(0,1);
    mirror  = regs[7].bit(1,2);
  }

  auto addressCIRAM(n32 address) -> n32 {
      switch (mirror) {
        case 0:
          // nametable is 0 0 0 1
          if (address & 0x0c00 != 0x0c00) {
            address = (n10)address;
          } else {
            address = 0x0400 | (n10)address;
          }
          break;

        case 1:
          // horizontal
          address = (address >> 1 & 0x0400) | (n10)address;
          break;

        case 2:
          // vertical
          address = (address >> 0 & 0x0400) | (n10)address;
          break;

        case 3:
          // single screen
          address = (n10)address;
          break;
      }

      return address;
  }

  n8 regs[8];
  n3 select;

  // 0b00 = 0 0 0 1, 0b01 = horizontal,
  // 0b10 = vertical, 0b11 = single screen
  n2 mirror;
  n2 prgBank;
  n4 chrBank;
};

