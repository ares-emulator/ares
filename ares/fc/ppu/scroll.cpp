auto PPU::incrementVRAMAddressX() -> void {
  if (++var.tileX == 0)
    var.nametableX++;
}

auto PPU::incrementVRAMAddressY() -> void {
  if (++var.fineY == 0 && ++var.tileY == 30)
    var.nametableY++, var.tileY = 0;
}

auto PPU::transferScrollX() -> void {
  var.tileX = scroll.tileX;
  var.nametableX = scroll.nametableX;
}

auto PPU::transferScrollY() -> void {
  var.tileY = scroll.tileY;
  var.fineY = scroll.fineY;
  var.nametableY = scroll.nametableY;
}

#if 1
template<u32 Cycle>
auto PPU::cycleScroll() -> void {
  if constexpr(Cycle ==   8) incrementVRAMAddressX();
  if constexpr(Cycle ==  16) incrementVRAMAddressX();
  if constexpr(Cycle ==  24) incrementVRAMAddressX();
  if constexpr(Cycle ==  32) incrementVRAMAddressX();
  if constexpr(Cycle ==  40) incrementVRAMAddressX();
  if constexpr(Cycle ==  48) incrementVRAMAddressX();
  if constexpr(Cycle ==  56) incrementVRAMAddressX();
  if constexpr(Cycle ==  64) incrementVRAMAddressX();
  if constexpr(Cycle ==  72) incrementVRAMAddressX();
  if constexpr(Cycle ==  80) incrementVRAMAddressX();
  if constexpr(Cycle ==  88) incrementVRAMAddressX();
  if constexpr(Cycle ==  96) incrementVRAMAddressX();
  if constexpr(Cycle == 104) incrementVRAMAddressX();
  if constexpr(Cycle == 112) incrementVRAMAddressX();
  if constexpr(Cycle == 120) incrementVRAMAddressX();
  if constexpr(Cycle == 128) incrementVRAMAddressX();
  if constexpr(Cycle == 136) incrementVRAMAddressX();
  if constexpr(Cycle == 144) incrementVRAMAddressX();
  if constexpr(Cycle == 152) incrementVRAMAddressX();
  if constexpr(Cycle == 160) incrementVRAMAddressX();
  if constexpr(Cycle == 168) incrementVRAMAddressX();
  if constexpr(Cycle == 176) incrementVRAMAddressX();
  if constexpr(Cycle == 184) incrementVRAMAddressX();
  if constexpr(Cycle == 192) incrementVRAMAddressX();
  if constexpr(Cycle == 200) incrementVRAMAddressX();
  if constexpr(Cycle == 208) incrementVRAMAddressX();
  if constexpr(Cycle == 216) incrementVRAMAddressX();
  if constexpr(Cycle == 224) incrementVRAMAddressX();
  if constexpr(Cycle == 232) incrementVRAMAddressX();
  if constexpr(Cycle == 240) incrementVRAMAddressX();
  if constexpr(Cycle == 248) incrementVRAMAddressX();
  if constexpr(Cycle == 256) incrementVRAMAddressX(), incrementVRAMAddressY();
  if constexpr(Cycle == 257) transferScrollX();
  if constexpr(Cycle == 280) if (io.ly == vlines() - 1) transferScrollY();
  if constexpr(Cycle == 281) if (io.ly == vlines() - 1) transferScrollY();
  if constexpr(Cycle == 282) if (io.ly == vlines() - 1) transferScrollY();
  if constexpr(Cycle == 283) if (io.ly == vlines() - 1) transferScrollY();
  if constexpr(Cycle == 284) if (io.ly == vlines() - 1) transferScrollY();
  if constexpr(Cycle == 285) if (io.ly == vlines() - 1) transferScrollY();
  if constexpr(Cycle == 286) if (io.ly == vlines() - 1) transferScrollY();
  if constexpr(Cycle == 287) if (io.ly == vlines() - 1) transferScrollY();
  if constexpr(Cycle == 288) if (io.ly == vlines() - 1) transferScrollY();
  if constexpr(Cycle == 289) if (io.ly == vlines() - 1) transferScrollY();
  if constexpr(Cycle == 290) if (io.ly == vlines() - 1) transferScrollY();
  if constexpr(Cycle == 291) if (io.ly == vlines() - 1) transferScrollY();
  if constexpr(Cycle == 292) if (io.ly == vlines() - 1) transferScrollY();
  if constexpr(Cycle == 293) if (io.ly == vlines() - 1) transferScrollY();
  if constexpr(Cycle == 294) if (io.ly == vlines() - 1) transferScrollY();
  if constexpr(Cycle == 295) if (io.ly == vlines() - 1) transferScrollY();
  if constexpr(Cycle == 296) if (io.ly == vlines() - 1) transferScrollY();
  if constexpr(Cycle == 297) if (io.ly == vlines() - 1) transferScrollY();
  if constexpr(Cycle == 298) if (io.ly == vlines() - 1) transferScrollY();
  if constexpr(Cycle == 299) if (io.ly == vlines() - 1) transferScrollY();
  if constexpr(Cycle == 300) if (io.ly == vlines() - 1) transferScrollY();
  if constexpr(Cycle == 301) if (io.ly == vlines() - 1) transferScrollY();
  if constexpr(Cycle == 302) if (io.ly == vlines() - 1) transferScrollY();
  if constexpr(Cycle == 303) if (io.ly == vlines() - 1) transferScrollY();
  if constexpr(Cycle == 304) if (io.ly == vlines() - 1) transferScrollY();
  if constexpr(Cycle == 328) incrementVRAMAddressX();
  if constexpr(Cycle == 336) incrementVRAMAddressX();
}
#endif

auto PPU::cycleScroll() -> void {
#define s(x) if (io.lx == x) cycleScroll<x>()
  s(8);   s(16);  s(24);  s(32);  s(40);  s(48);  s(56);  s(64);
  s(72);  s(80);  s(88);  s(96);  s(104); s(112); s(120); s(128);
  s(136); s(144); s(152); s(160); s(168); s(176); s(184); s(192);
  s(200); s(208); s(216); s(224); s(232); s(240); s(248); s(256);
  s(257); s(280); s(281); s(282); s(283); s(284); s(285); s(286);
  s(287); s(288); s(289); s(290); s(291); s(292); s(293); s(294);
  s(295); s(296); s(297); s(298); s(299); s(300); s(301); s(302);
  s(303); s(304); s(328); s(336);
#undef s
}

