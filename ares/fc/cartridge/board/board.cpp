namespace Board {

#include "bandai-fcg.cpp"
#include "jaleco-jf.cpp"
#include "jaleco-jf11-jf14.cpp"
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
#include "hvc-dxrom.cpp"
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
#include "irem-tam-s1.cpp"
#include "sunsoft-1.cpp"
#include "sunsoft-2.cpp"
#include "sunsoft-3.cpp"
#include "sunsoft-4.cpp"
#include "sunsoft-5b.cpp"

auto Interface::create(string board) -> Interface* {
  Interface* p = nullptr;
  if(!p) p = BandaiFCG::create(board);
  if(!p) p = HVC_AxROM::create(board);
  if(!p) p = HVC_BNROM::create(board);
  if(!p) p = HVC_CNROM::create(board);
  if(!p) p = HVC_DxROM::create(board);
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
  if(!p) p = IremTAMS1::create(board);
  if(!p) p = JalecoJF::create(board);
  if(!p) p = Jaleco_JF11_JF14::create(board);
  if(!p) p = KonamiVRC1::create(board);
  if(!p) p = KonamiVRC2::create(board);
  if(!p) p = KonamiVRC3::create(board);
  if(!p) p = KonamiVRC4::create(board);
  if(!p) p = KonamiVRC5::create(board);
  if(!p) p = KonamiVRC6::create(board);
  if(!p) p = KonamiVRC7::create(board);
  if(!p) p = Sunsoft1::create(board);
  if(!p) p = Sunsoft2::create(board);
  if(!p) p = Sunsoft3::create(board);
  if(!p) p = Sunsoft4::create(board);
  if(!p) p = Sunsoft5B::create(board);
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
