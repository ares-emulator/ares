#include "bandai-fcg.cpp"
#include "hvc-fmr.cpp"
#include "konami-vrc1.cpp"
#include "konami-vrc2.cpp"
#include "konami-vrc3.cpp"
#include "konami-vrc4.cpp"
#include "konami-vrc5.cpp"
#include "konami-vrc6.cpp"
#include "konami-vrc7.cpp"
#include "nes-axrom.cpp"
#include "nes-bnrom.cpp"
#include "nes-cnrom.cpp"
#include "nes-exrom.cpp"
#include "nes-fxrom.cpp"
#include "nes-gxrom.cpp"
#include "nes-hkrom.cpp"
#include "nes-nrom.cpp"
#include "nes-pxrom.cpp"
#include "nes-sxrom.cpp"
#include "nes-txrom.cpp"
#include "nes-uxrom.cpp"
#include "sunsoft-5b.cpp"

Board::Board(Markup::Node document) {
  cartridge.board = this;
  information.type = document["game/board"].text();
}

auto Board::load() -> void {
  auto document = BML::unserialize(cartridge.manifest());
  load(document);
}

auto Board::save() -> void {
  auto document = BML::unserialize(cartridge.manifest());
  save(document);
}

auto Board::unload() -> void {
}

auto Board::main() -> void {
  cartridge.step(cartridge.rate() * 4095);
  tick();
}

auto Board::tick() -> void {
  cartridge.step(cartridge.rate());
  cartridge.synchronize(cpu);
}

auto Board::load(string manifest) -> Board* {
  auto document = BML::unserialize(manifest);

  string type = document["game/board"].text();

  if(type == "BANDAI-FCG"  ) return new BandaiFCG(document);

  if(type == "HVC-FMR"     ) return new HVC_FMR(document);

  if(type == "KONAMI-VRC-1") return new KonamiVRC1(document);
  if(type == "KONAMI-VRC-2") return new KonamiVRC2(document);
  if(type == "KONAMI-VRC-3") return new KonamiVRC3(document);
  if(type == "KONAMI-VRC-4") return new KonamiVRC4(document);
  if(type == "KONAMI-VRC-5") return new KonamiVRC5(document);
  if(type == "KONAMI-VRC-6") return new KonamiVRC6(document);
  if(type == "KONAMI-VRC-7") return new KonamiVRC7(document);

  if(type == "NES-AMROM"   ) return new NES_AxROM(document, NES_AxROM::Revision::AMROM);
  if(type == "NES-ANROM"   ) return new NES_AxROM(document, NES_AxROM::Revision::ANROM);
  if(type == "NES-AN1ROM"  ) return new NES_AxROM(document, NES_AxROM::Revision::AN1ROM);
  if(type == "NES-AOROM"   ) return new NES_AxROM(document, NES_AxROM::Revision::AOROM);

  if(type == "NES-BNROM"   ) return new NES_BNROM(document);

  if(type == "NES-CNROM"   ) return new NES_CNROM(document);

  if(type == "NES-EKROM"   ) return new NES_ExROM(document, NES_ExROM::Revision::EKROM);
  if(type == "NES-ELROM"   ) return new NES_ExROM(document, NES_ExROM::Revision::ELROM);
  if(type == "NES-ETROM"   ) return new NES_ExROM(document, NES_ExROM::Revision::ETROM);
  if(type == "NES-EWROM"   ) return new NES_ExROM(document, NES_ExROM::Revision::EWROM);

  if(type == "NES-FJROM"   ) return new NES_FxROM(document, NES_FxROM::Revision::FJROM);
  if(type == "NES-FKROM"   ) return new NES_FxROM(document, NES_FxROM::Revision::FKROM);

  if(type == "NES-GNROM"   ) return new NES_GxROM(document, NES_GxROM::Revision::GNROM);
  if(type == "NES-MHROM"   ) return new NES_GxROM(document, NES_GxROM::Revision::MHROM);

  if(type == "NES-HKROM"   ) return new NES_HKROM(document);

  if(type == "NES-NROM"    ) return new NES_NROM(document);
  if(type == "NES-NROM-128") return new NES_NROM(document);
  if(type == "NES-NROM-256") return new NES_NROM(document);

  if(type == "NES-PEEOROM" ) return new NES_PxROM(document, NES_PxROM::Revision::PEEOROM);
  if(type == "NES-PNROM"   ) return new NES_PxROM(document, NES_PxROM::Revision::PNROM);

  if(type == "NES-SAROM"   ) return new NES_SxROM(document, NES_SxROM::Revision::SAROM);
  if(type == "NES-SBROM"   ) return new NES_SxROM(document, NES_SxROM::Revision::SBROM);
  if(type == "NES-SCROM"   ) return new NES_SxROM(document, NES_SxROM::Revision::SCROM);
  if(type == "NES-SC1ROM"  ) return new NES_SxROM(document, NES_SxROM::Revision::SC1ROM);
  if(type == "NES-SEROM"   ) return new NES_SxROM(document, NES_SxROM::Revision::SEROM);
  if(type == "NES-SFROM"   ) return new NES_SxROM(document, NES_SxROM::Revision::SFROM);
  if(type == "NES-SFEXPROM") return new NES_SxROM(document, NES_SxROM::Revision::SFEXPROM);
  if(type == "NES-SGROM"   ) return new NES_SxROM(document, NES_SxROM::Revision::SGROM);
  if(type == "NES-SHROM"   ) return new NES_SxROM(document, NES_SxROM::Revision::SHROM);
  if(type == "NES-SH1ROM"  ) return new NES_SxROM(document, NES_SxROM::Revision::SH1ROM);
  if(type == "NES-SIROM"   ) return new NES_SxROM(document, NES_SxROM::Revision::SIROM);
  if(type == "NES-SJROM"   ) return new NES_SxROM(document, NES_SxROM::Revision::SJROM);
  if(type == "NES-SKROM"   ) return new NES_SxROM(document, NES_SxROM::Revision::SKROM);
  if(type == "NES-SLROM"   ) return new NES_SxROM(document, NES_SxROM::Revision::SKROM);
  if(type == "NES-SL1ROM"  ) return new NES_SxROM(document, NES_SxROM::Revision::SL1ROM);
  if(type == "NES-SL2ROM"  ) return new NES_SxROM(document, NES_SxROM::Revision::SL2ROM);
  if(type == "NES-SL3ROM"  ) return new NES_SxROM(document, NES_SxROM::Revision::SL3ROM);
  if(type == "NES-SLRROM"  ) return new NES_SxROM(document, NES_SxROM::Revision::SLRROM);
  if(type == "NES-SMROM"   ) return new NES_SxROM(document, NES_SxROM::Revision::SMROM);
  if(type == "NES-SNROM"   ) return new NES_SxROM(document, NES_SxROM::Revision::SNROM);
  if(type == "NES-SOROM"   ) return new NES_SxROM(document, NES_SxROM::Revision::SOROM);
  if(type == "NES-SUROM"   ) return new NES_SxROM(document, NES_SxROM::Revision::SUROM);
  if(type == "NES-SXROM"   ) return new NES_SxROM(document, NES_SxROM::Revision::SXROM);

  if(type == "NES-TBROM"   ) return new NES_TxROM(document, NES_TxROM::Revision::TBROM);
  if(type == "NES-TEROM"   ) return new NES_TxROM(document, NES_TxROM::Revision::TEROM);
  if(type == "NES-TFROM"   ) return new NES_TxROM(document, NES_TxROM::Revision::TFROM);
  if(type == "NES-TGROM"   ) return new NES_TxROM(document, NES_TxROM::Revision::TGROM);
  if(type == "NES-TKROM"   ) return new NES_TxROM(document, NES_TxROM::Revision::TKROM);
  if(type == "NES-TKSROM"  ) return new NES_TxROM(document, NES_TxROM::Revision::TKSROM);
  if(type == "NES-TLROM"   ) return new NES_TxROM(document, NES_TxROM::Revision::TLROM);
  if(type == "NES-TL1ROM"  ) return new NES_TxROM(document, NES_TxROM::Revision::TL1ROM);
  if(type == "NES-TL2ROM"  ) return new NES_TxROM(document, NES_TxROM::Revision::TL2ROM);
  if(type == "NES-TLSROM"  ) return new NES_TxROM(document, NES_TxROM::Revision::TLSROM);
  if(type == "NES-TNROM"   ) return new NES_TxROM(document, NES_TxROM::Revision::TNROM);
  if(type == "NES-TQROM"   ) return new NES_TxROM(document, NES_TxROM::Revision::TQROM);
  if(type == "NES-TR1ROM"  ) return new NES_TxROM(document, NES_TxROM::Revision::TR1ROM);
  if(type == "NES-TSROM"   ) return new NES_TxROM(document, NES_TxROM::Revision::TSROM);
  if(type == "NES-TVROM"   ) return new NES_TxROM(document, NES_TxROM::Revision::TVROM);

  if(type == "NES-UNROM"   ) return new NES_UxROM(document, NES_UxROM::Revision::UNROM);
  if(type == "NES-UOROM"   ) return new NES_UxROM(document, NES_UxROM::Revision::UOROM);

  if(type == "SUNSOFT-5B"  ) return new Sunsoft5B(document);

  return nullptr;
}

auto Board::load(Memory::Readable<uint8>& memory, Markup::Node node) -> bool {
  if(!node) return false;
  memory.allocate(node["size"].natural(), 0xff);
  auto filename = string{node["content"].string(), ".", node["type"].string()}.downcase();
  if(auto fp = platform->open(cartridge.node, filename, File::Read)) {
    memory.load(fp);
    return true;
  }
  return false;
}

auto Board::load(Memory::Writable<uint8>& memory, Markup::Node node) -> bool {
  if(!node) return false;
  memory.allocate(node["size"].natural(), 0xff);
  if(node["volatile"]) return true;
  auto filename = string{node["content"].string(), ".", node["type"].string()}.downcase();
  if(auto fp = platform->open(cartridge.node, filename, File::Read)) {
    memory.load(fp);
    return true;
  }
  return false;
}

auto Board::save(Memory::Writable<uint8>& memory, Markup::Node node) -> bool {
  if(!node) return false;
  if(node["volatile"]) return true;
  auto filename = string{node["content"].string(), ".", node["type"].string()}.downcase();
  if(auto fp = platform->open(cartridge.node, filename, File::Write)) {
    memory.save(fp);
    return true;
  }
  return false;
}
