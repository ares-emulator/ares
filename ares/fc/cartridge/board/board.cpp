namespace Board {

#include "bandai-fcg.cpp"
#include "konami-vrc1.cpp"
#include "konami-vrc2.cpp"
#include "konami-vrc3.cpp"
#include "konami-vrc4.cpp"
#include "konami-vrc5.cpp"
#include "konami-vrc6.cpp"
#include "konami-vrc7.cpp"
#include "hvc-axrom.cpp"
#include "hvc-bnrom.cpp"
#include "hvc-cnrom.cpp"
#include "hvc-exrom.cpp"
#include "hvc-fmr.cpp"
#include "hvc-fxrom.cpp"
#include "hvc-gxrom.cpp"
#include "hvc-hkrom.cpp"
#include "hvc-nrom.cpp"
#include "hvc-pxrom.cpp"
#include "hvc-sxrom.cpp"
#include "hvc-txrom.cpp"
#include "hvc-uxrom.cpp"
#include "sunsoft-5b.cpp"

Interface::Interface(Markup::Node document) {
  cartridge.board = this;
  information.type = document["game/board"].text();
}

auto Interface::load() -> void {
  auto document = BML::unserialize(cartridge.manifest());
  load(document);
}

auto Interface::save() -> void {
  auto document = BML::unserialize(cartridge.manifest());
  save(document);
}

auto Interface::unload() -> void {
}

auto Interface::main() -> void {
  cartridge.step(cartridge.rate() * 4095);
  tick();
}

auto Interface::tick() -> void {
  cartridge.step(cartridge.rate());
  cartridge.synchronize(cpu);
}

auto Interface::load(string manifest) -> Interface* {
  auto document = BML::unserialize(manifest);

  string type = document["game/board"].text();

  if(type == "BANDAI-FCG"  ) return new BandaiFCG(document);

  if(type == "HVC-AMROM"   ) return new HVC_AxROM(document, HVC_AxROM::Revision::AMROM);
  if(type == "HVC-ANROM"   ) return new HVC_AxROM(document, HVC_AxROM::Revision::ANROM);
  if(type == "HVC-AN1ROM"  ) return new HVC_AxROM(document, HVC_AxROM::Revision::AN1ROM);
  if(type == "HVC-AOROM"   ) return new HVC_AxROM(document, HVC_AxROM::Revision::AOROM);

  if(type == "HVC-BNROM"   ) return new HVC_BNROM(document);

  if(type == "HVC-CNROM"   ) return new HVC_CNROM(document);

  if(type == "HVC-EKROM"   ) return new HVC_ExROM(document, HVC_ExROM::Revision::EKROM);
  if(type == "HVC-ELROM"   ) return new HVC_ExROM(document, HVC_ExROM::Revision::ELROM);
  if(type == "HVC-ETROM"   ) return new HVC_ExROM(document, HVC_ExROM::Revision::ETROM);
  if(type == "HVC-EWROM"   ) return new HVC_ExROM(document, HVC_ExROM::Revision::EWROM);

  if(type == "HVC-FMR"     ) return new HVC_FMR(document);

  if(type == "HVC-FJROM"   ) return new HVC_FxROM(document, HVC_FxROM::Revision::FJROM);
  if(type == "HVC-FKROM"   ) return new HVC_FxROM(document, HVC_FxROM::Revision::FKROM);

  if(type == "HVC-GNROM"   ) return new HVC_GxROM(document, HVC_GxROM::Revision::GNROM);
  if(type == "HVC-MHROM"   ) return new HVC_GxROM(document, HVC_GxROM::Revision::MHROM);

  if(type == "HVC-HKROM"   ) return new HVC_HKROM(document);

  if(type == "HVC-NROM"    ) return new HVC_NROM(document);
  if(type == "HVC-NROM-128") return new HVC_NROM(document);
  if(type == "HVC-NROM-256") return new HVC_NROM(document);

  if(type == "HVC-PEEOROM" ) return new HVC_PxROM(document, HVC_PxROM::Revision::PEEOROM);
  if(type == "HVC-PNROM"   ) return new HVC_PxROM(document, HVC_PxROM::Revision::PNROM);

  if(type == "HVC-SAROM"   ) return new HVC_SxROM(document, HVC_SxROM::Revision::SAROM);
  if(type == "HVC-SBROM"   ) return new HVC_SxROM(document, HVC_SxROM::Revision::SBROM);
  if(type == "HVC-SCROM"   ) return new HVC_SxROM(document, HVC_SxROM::Revision::SCROM);
  if(type == "HVC-SC1ROM"  ) return new HVC_SxROM(document, HVC_SxROM::Revision::SC1ROM);
  if(type == "HVC-SEROM"   ) return new HVC_SxROM(document, HVC_SxROM::Revision::SEROM);
  if(type == "HVC-SFROM"   ) return new HVC_SxROM(document, HVC_SxROM::Revision::SFROM);
  if(type == "HVC-SFEXPROM") return new HVC_SxROM(document, HVC_SxROM::Revision::SFEXPROM);
  if(type == "HVC-SGROM"   ) return new HVC_SxROM(document, HVC_SxROM::Revision::SGROM);
  if(type == "HVC-SHROM"   ) return new HVC_SxROM(document, HVC_SxROM::Revision::SHROM);
  if(type == "HVC-SH1ROM"  ) return new HVC_SxROM(document, HVC_SxROM::Revision::SH1ROM);
  if(type == "HVC-SIROM"   ) return new HVC_SxROM(document, HVC_SxROM::Revision::SIROM);
  if(type == "HVC-SJROM"   ) return new HVC_SxROM(document, HVC_SxROM::Revision::SJROM);
  if(type == "HVC-SKROM"   ) return new HVC_SxROM(document, HVC_SxROM::Revision::SKROM);
  if(type == "HVC-SLROM"   ) return new HVC_SxROM(document, HVC_SxROM::Revision::SKROM);
  if(type == "HVC-SL1ROM"  ) return new HVC_SxROM(document, HVC_SxROM::Revision::SL1ROM);
  if(type == "HVC-SL2ROM"  ) return new HVC_SxROM(document, HVC_SxROM::Revision::SL2ROM);
  if(type == "HVC-SL3ROM"  ) return new HVC_SxROM(document, HVC_SxROM::Revision::SL3ROM);
  if(type == "HVC-SLRROM"  ) return new HVC_SxROM(document, HVC_SxROM::Revision::SLRROM);
  if(type == "HVC-SMROM"   ) return new HVC_SxROM(document, HVC_SxROM::Revision::SMROM);
  if(type == "HVC-SNROM"   ) return new HVC_SxROM(document, HVC_SxROM::Revision::SNROM);
  if(type == "HVC-SOROM"   ) return new HVC_SxROM(document, HVC_SxROM::Revision::SOROM);
  if(type == "HVC-SUROM"   ) return new HVC_SxROM(document, HVC_SxROM::Revision::SUROM);
  if(type == "HVC-SXROM"   ) return new HVC_SxROM(document, HVC_SxROM::Revision::SXROM);

  if(type == "HVC-TBROM"   ) return new HVC_TxROM(document, HVC_TxROM::Revision::TBROM);
  if(type == "HVC-TEROM"   ) return new HVC_TxROM(document, HVC_TxROM::Revision::TEROM);
  if(type == "HVC-TFROM"   ) return new HVC_TxROM(document, HVC_TxROM::Revision::TFROM);
  if(type == "HVC-TGROM"   ) return new HVC_TxROM(document, HVC_TxROM::Revision::TGROM);
  if(type == "HVC-TKROM"   ) return new HVC_TxROM(document, HVC_TxROM::Revision::TKROM);
  if(type == "HVC-TKSROM"  ) return new HVC_TxROM(document, HVC_TxROM::Revision::TKSROM);
  if(type == "HVC-TLROM"   ) return new HVC_TxROM(document, HVC_TxROM::Revision::TLROM);
  if(type == "HVC-TL1ROM"  ) return new HVC_TxROM(document, HVC_TxROM::Revision::TL1ROM);
  if(type == "HVC-TL2ROM"  ) return new HVC_TxROM(document, HVC_TxROM::Revision::TL2ROM);
  if(type == "HVC-TLSROM"  ) return new HVC_TxROM(document, HVC_TxROM::Revision::TLSROM);
  if(type == "HVC-TNROM"   ) return new HVC_TxROM(document, HVC_TxROM::Revision::TNROM);
  if(type == "HVC-TQROM"   ) return new HVC_TxROM(document, HVC_TxROM::Revision::TQROM);
  if(type == "HVC-TR1ROM"  ) return new HVC_TxROM(document, HVC_TxROM::Revision::TR1ROM);
  if(type == "HVC-TSROM"   ) return new HVC_TxROM(document, HVC_TxROM::Revision::TSROM);
  if(type == "HVC-TVROM"   ) return new HVC_TxROM(document, HVC_TxROM::Revision::TVROM);

  if(type == "HVC-UNROM"   ) return new HVC_UxROM(document, HVC_UxROM::Revision::UNROM);
  if(type == "HVC-UOROM"   ) return new HVC_UxROM(document, HVC_UxROM::Revision::UOROM);

  if(type == "KONAMI-VRC-1") return new KonamiVRC1(document);
  if(type == "KONAMI-VRC-2") return new KonamiVRC2(document);
  if(type == "KONAMI-VRC-3") return new KonamiVRC3(document);
  if(type == "KONAMI-VRC-4") return new KonamiVRC4(document);
  if(type == "KONAMI-VRC-5") return new KonamiVRC5(document);
  if(type == "KONAMI-VRC-6") return new KonamiVRC6(document);
  if(type == "KONAMI-VRC-7") return new KonamiVRC7(document);

  if(type == "SUNSOFT-5B"  ) return new Sunsoft5B(document);

  //fallback for when no cartridge is inserted or an unsupported mapper is specified
  return new Interface(document);
}

auto Interface::load(Memory::Readable<uint8>& memory, Markup::Node node) -> bool {
  if(!node) return false;
  memory.allocate(node["size"].natural(), 0xff);
  auto filename = string{node["content"].string(), ".", node["type"].string()}.downcase();
  if(auto fp = platform->open(cartridge.node, filename, File::Read)) {
    memory.load(fp);
    return true;
  }
  return false;
}

auto Interface::load(Memory::Writable<uint8>& memory, Markup::Node node) -> bool {
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

auto Interface::save(Memory::Writable<uint8>& memory, Markup::Node node) -> bool {
  if(!node) return false;
  if(node["volatile"]) return true;
  auto filename = string{node["content"].string(), ".", node["type"].string()}.downcase();
  if(auto fp = platform->open(cartridge.node, filename, File::Write)) {
    memory.save(fp);
    return true;
  }
  return false;
}

}
