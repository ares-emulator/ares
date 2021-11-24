namespace Board {

#include "bandai-74161.cpp"
#include "bandai-fcg.cpp"
#include "bandai-karaoke.cpp"
#include "bandai-lz93d50.cpp"
#include "colordreams-74x377.cpp"
#include "gtrom.cpp"
#include "jaleco-jf05.cpp"
#include "jaleco-jf11.cpp"
#include "jaleco-jf16.cpp"
#include "jaleco-jf17.cpp"
#include "jaleco-jf23.cpp"
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
#include "irem-g101.cpp"
#include "irem-h3001.cpp"
#include "irem-lrog017.cpp"
#include "irem-tam-s1.cpp"
#include "mlt-action52.cpp"
#include "namco-118.cpp"
#include "namco-163.cpp"
#include "namco-340.cpp"
#include "sunsoft-1.cpp"
#include "sunsoft-2.cpp"
#include "sunsoft-3.cpp"
#include "sunsoft-4.cpp"
#include "sunsoft-5b.cpp"
#include "taito-tc0190.cpp"
#include "taito-tc0690.cpp"
#include "taito-x1-005.cpp"
#include "taito-x1-017.cpp"

auto Interface::create(string board) -> Interface* {
  Interface* p = nullptr;
  if(!p) p = Bandai74161::create(board);
  if(!p) p = BandaiFCG::create(board);
  if(!p) p = BandaiKaraoke::create(board);
  if(!p) p = BandaiLZ93D50::create(board);
  if(!p) p = ColorDreams_74x377::create(board);
  if(!p) p = GTROM::create(board);
  if(!p) p = HVC_AxROM::create(board);
  if(!p) p = HVC_BNROM::create(board);
  if(!p) p = HVC_CNROM::create(board);
  if(!p) p = HVC_ExROM::create(board);
  if(!p) p = HVC_FMR::create(board);
  if(!p) p = HVC_FxROM::create(board);
  if(!p) p = HVC_GxROM::create(board);
  if(!p) p = HVC_HKROM::create(board);
  if(!p) p = HVC_NROM::create(board);
  if(!p) p = HVC_PxROM::create(board);
  if(!p) p = HVC_SxROM::create(board);
  if(!p) p = HVC_TxROM::create(board);
  if(!p) p = HVC_UxROM::create(board);
  if(!p) p = IremG101::create(board);
  if(!p) p = IremH3001::create(board);
  if(!p) p = IremLROG017::create(board);
  if(!p) p = IremTAMS1::create(board);
  if(!p) p = JalecoJF05::create(board);
  if(!p) p = JalecoJF11::create(board);
  if(!p) p = JalecoJF16::create(board);
  if(!p) p = JalecoJF17::create(board);
  if(!p) p = JalecoJF23::create(board);
  if(!p) p = KonamiVRC1::create(board);
  if(!p) p = KonamiVRC2::create(board);
  if(!p) p = KonamiVRC3::create(board);
  if(!p) p = KonamiVRC4::create(board);
  if(!p) p = KonamiVRC5::create(board);
  if(!p) p = KonamiVRC6::create(board);
  if(!p) p = KonamiVRC7::create(board);
  if(!p) p = MltAction52::create(board);
  if(!p) p = Namco118::create(board);
  if(!p) p = Namco163::create(board);
  if(!p) p = Namco340::create(board);
  if(!p) p = Sunsoft1::create(board);
  if(!p) p = Sunsoft2::create(board);
  if(!p) p = Sunsoft3::create(board);
  if(!p) p = Sunsoft4::create(board);
  if(!p) p = Sunsoft5B::create(board);
  if(!p) p = TaitoTC0190::create(board);
  if(!p) p = TaitoTC0690::create(board);
  if(!p) p = TaitoX1005::create(board);
  if(!p) p = TaitoX1017::create(board);
  if(!p) p = new Interface;
  return p;
}

auto Interface::main() -> void {
  cartridge.step(cartridge.rate() * 4095);
  tick();
}

auto Interface::tick() -> void {
  cartridge.step(cartridge.rate());
  cartridge.synchronize(cpu);
}

auto Interface::load(Memory::Readable<n8>& memory, string name) -> bool {
  if(auto fp = pak->read(name)) {
    memory.allocate(fp->size(), 0xff);
    memory.load(fp);
    return true;
  }
  return false;
}

auto Interface::load(Memory::Writable<n8>& memory, string name) -> bool {
  if(auto fp = pak->read(name)) {
    memory.allocate(fp->size(), 0xff);
    memory.load(fp);
    return true;
  }
  return false;
}

auto Interface::save(Memory::Writable<n8>& memory, string name) -> bool {
  if(auto fp = pak->write(name)) {
    memory.save(fp);
    return true;
  }
  return false;
}

}
