// Decryption algorithms heavily based on/borrowed MAME
// See https://github.com/mamedev/mame/tree/master/src/devices/bus/neogeo
// and the MAME section in LICENCE in the ares license text.

struct NeoGeo : Mame {
  auto name() -> string override { return "Neo Geo"; }
  auto extensions() -> vector<string> override { return {"ng"}; }
  auto read(string location, string match) -> vector<u8>;
  auto load(string location) -> bool override;
  auto board() -> string;
  auto save(string location) -> bool override;
  auto analyze(vector<u8>& p, vector<u8>& m, vector<u8>& c, vector<u8>& s, vector<u8>& vA, vector<u8>& vB) -> string;
  auto decrypt(vector<u8>& p, vector<u8>& m, vector<u8>& c, vector<u8>& s, vector<u8>& vA, vector<u8>& vB) -> void;
  auto decryptCmc42(vector<u8>& c , vector<u8>& s, u8 key) -> void;
  auto decryptCmc50(vector<u8>& c , vector<u8>& s, vector<u8>& m, u8 key) -> void;
  auto loadCmcFixedRom(vector<u8>& c, vector<u8>& s) -> void;
  auto decryptCmcGraphics(vector<u8>& c, u8 key) -> void;
  auto decryptCmcGraphicsInternal(u8 *r0, u8 *r1, u8 c0,  u8 c1, u8 *table0hi,u8 *table0lo,
                                  u8 *table1, int base, int invert) -> void;
  auto decryptCmcMusic(vector<u8>& m) -> void;
  auto decryptCmcMusicDescramble(u32 addr, u16 key) -> u32;

  Markup::Node info;

  #include "neo-geo-crypt.hpp"
  struct CmcContext {
    u8 *type0_t03;
    u8 *type0_t12;
    u8 *type1_t03;
    u8 *type1_t12;
    u8 *address_8_15_xor1;
    u8 *address_8_15_xor2;
    u8 *address_16_23_xor1;
    u8 *address_16_23_xor2;
    u8 *address_0_7_xor;
  } cmc;
};

auto NeoGeo::read(string location, string match) -> vector<u8> {
  // we expect mame style .zip rom images
  if(!location.iendsWith(".zip")) {}

  if(info) {
    if(match == "program.rom")   return loadRoms(location, info, "maincpu");
    if(match == "character.rom") return loadRoms(location, info, "sprites");
    if(match == "static.rom")    return loadRoms(location, info, "fixed");
    if(match == "voice-a.rom")   return loadRoms(location, info, "ymsndadpcma");
    if(match == "voice-b.rom")   return loadRoms(location, info, "ymsndadpcmb");
    if(match == "music.rom") {
      // music rom can be plaintext (audiocpu) or encrypted (audiocrypt)
      // we must load both types
      auto music = loadRoms(location, info, "audiocpu");
      if(music.size() == 0) return loadRoms(location, info, "audiocrypt");
      return music;
    }
  }

  return {};
}

auto NeoGeo::load(string location) -> bool {
  vector<u8> programROM;    //P ROM (68K CPU program)
  vector<u8> musicROM;      //M ROM (Z80 APU program)
  vector<u8> characterROM;  //C ROM (sprite and background character graphics)
  vector<u8> staticROM;     //S ROM (fix layer static graphics)
  vector<u8> voiceAROM;     //V ROM (ADPCM-A voice samples)
  vector<u8> voiceBROM;     //V ROM (ADPCM-B voice samples)

  this->info = BML::unserialize(manifestDatabaseArcade(Medium::name(location)));

  if(file::exists(location)) {
    programROM   = NeoGeo::read(location, "program.rom");
    musicROM     = NeoGeo::read(location, "music.rom");
    characterROM = NeoGeo::read(location, "character.rom");
    staticROM    = NeoGeo::read(location, "static.rom");
    voiceAROM    = NeoGeo::read(location, "voice-a.rom");
    voiceBROM    = NeoGeo::read(location, "voice-b.rom");
  }

  if(!programROM  ) return false;
  if(!musicROM    ) return false;
  if(!characterROM) return false;
  if(!staticROM   ) return false;
  if(!voiceAROM   ) return false;
  //voiceB is optional

  //many games have encrypted roms, so let's decrypt them here
  decrypt(programROM, musicROM, characterROM, staticROM, voiceAROM, voiceBROM);

  Hash::SHA256 hash;
  hash.input(programROM);
  hash.input(musicROM);
  hash.input(characterROM);
  hash.input(staticROM);
  hash.input(voiceAROM);
  hash.input(voiceBROM);
  auto sha256 = hash.digest();

  this->location = location;
  this->manifest = analyze(programROM, musicROM, characterROM, staticROM, voiceAROM, voiceBROM);
  auto document = BML::unserialize(manifest);
  if(!document) return false;

  pak = new vfs::directory;
  pak->setAttribute("sha256",  sha256);
  pak->setAttribute("title",   document["game/title"].string());
  pak->setAttribute("board",   document["game/board"].string());
  pak->append("manifest.bml",  manifest);
  pak->append("program.rom",   programROM);
  pak->append("music.rom",     musicROM);
  pak->append("character.rom", characterROM);
  pak->append("static.rom",    staticROM);
  pak->append("voice-a.rom",   voiceAROM);
  pak->append("voice-b.rom",   voiceBROM);
  return true;
}

auto NeoGeo::save(string location) -> bool {
  auto document = BML::unserialize(manifest);

  return true;
}

auto NeoGeo::board() -> string {
  if(info) {
    for (auto element: info["game"]) {
      if(element.name() == "feature" && element["name"].string() == "slot") return element["value"].string();
    }
  }

  return "rom";
}

auto NeoGeo::analyze(vector<u8>& p, vector<u8>& m, vector<u8>& c, vector<u8>& s, vector<u8>& vA, vector<u8>& vB) -> string {
  string manifest;

  manifest += "game\n";
  manifest +={"  name:   ", Medium::name(location), "\n"};
  manifest +={"  title:  ", (info ? info["game/title"].string() : Medium::name(location)), "\n"};
  manifest +={"  board:  ", board(), "\n"};
  manifest +={"    memory type=ROM size=0x", hex( p.size(), 8L), " content=Program\n"};
  manifest +={"    memory type=ROM size=0x", hex( m.size(), 8L), " content=Music\n"};
  manifest +={"    memory type=ROM size=0x", hex( c.size(), 8L), " content=Character\n"};
  manifest +={"    memory type=ROM size=0x", hex( s.size(), 8L), " content=Static\n"};
  manifest +={"    memory type=ROM size=0x", hex(vA.size(), 8L), " content=VoiceA\n"};
  manifest +={"    memory type=ROM size=0x", hex(vB.size(), 8L), " content=VoiceB\n"};
  return manifest;
}

auto NeoGeo::decrypt(vector<u8>& p, vector<u8>& m, vector<u8>& c, vector<u8>& s, vector<u8>& vA, vector<u8>& vB) -> void {
  //cmc42
  if(board() == "cmc42_bangbead") return decryptCmc42(c, s, 0xf8);
  if(board() == "cmc42_ganryu"  ) return decryptCmc42(c, s, 0x07);
  if(board() == "cmc42_kof99k"  ) return decryptCmc42(c, s, 0x00);
  if(board() == "cmc42_mslug3h" ) return decryptCmc42(c, s, 0xad);
  if(board() == "cmc42_nitd"    ) return decryptCmc42(c, s, 0xff);
  if(board() == "cmc42_preisle2") return decryptCmc42(c, s, 0x9f);
  if(board() == "cmc42_s1945p"  ) return decryptCmc42(c, s, 0x05);
  if(board() == "cmc42_sengoku3") return decryptCmc42(c, s, 0xfe);
  if(board() == "cmc42_zupapa"  ) return decryptCmc42(c, s, 0xbd);
  //cmc50
  if(board() == "cmc50_jockeygp") return decryptCmc50(c, s, m, 0xac);
  if(board() == "cmc50_kof2001" ) return decryptCmc50(c, s, m, 0x1e);
  if(board() == "cmc50_kof2000n") return decryptCmc50(c, s, m, 0x00);
}

auto NeoGeo::decryptCmc42(vector<u8>& c, vector<u8>& s, u8 key) -> void {
  cmc.type0_t03          = kof99_type0_t03;
  cmc.type0_t12          = kof99_type0_t12;
  cmc.type1_t03          = kof99_type1_t03;
  cmc.type1_t12          = kof99_type1_t12;
  cmc.address_8_15_xor1  = kof99_address_8_15_xor1;
  cmc.address_8_15_xor2  = kof99_address_8_15_xor2;
  cmc.address_16_23_xor1 = kof99_address_16_23_xor1;
  cmc.address_16_23_xor2 = kof99_address_16_23_xor2;
  cmc.address_0_7_xor    = kof99_address_0_7_xor;

  decryptCmcGraphics(c, key);
  loadCmcFixedRom(c, s);
}

auto NeoGeo::decryptCmc50(vector<u8>& c, vector<u8>& s, vector<u8>& m, u8 key) -> void {
  cmc.type0_t03          = kof2000_type0_t03;
  cmc.type0_t12          = kof2000_type0_t12;
  cmc.type1_t03          = kof2000_type1_t03;
  cmc.type1_t12          = kof2000_type1_t12;
  cmc.address_8_15_xor1  = kof2000_address_8_15_xor1;
  cmc.address_8_15_xor2  = kof2000_address_8_15_xor2;
  cmc.address_16_23_xor1 = kof2000_address_16_23_xor1;
  cmc.address_16_23_xor2 = kof2000_address_16_23_xor2;
  cmc.address_0_7_xor    = kof2000_address_0_7_xor;

  decryptCmcGraphics(c, key);
  loadCmcFixedRom(c, s);
  decryptCmcMusic(m);
}

auto NeoGeo::loadCmcFixedRom(vector<u8>& c, vector<u8>& s) -> void {
  // SROM is stored after CROM
  for (int i = 0; i < s.size(); i++) {
    s[i] = c[(c.size() - s.size()) + ((i & ~0x1f) + ((i & 7) << 2) + ((~i & 8) >> 2) + ((i & 0x10) >> 4))];
  }
}

auto NeoGeo::decryptCmcGraphics(vector<u8>& c, u8 key) -> void {
  vector<u8> buf;
  buf.resize(c.size());

  // Data xor
  for (auto rpos = 0; rpos < c.size() / 4; rpos++) {
    decryptCmcGraphicsInternal(&buf[4 * rpos + 0], &buf[4 * rpos + 3], c[4 * rpos+0], c[4 * rpos+3],
            cmc. type0_t03, cmc.type0_t12, cmc.type1_t03, rpos, (rpos >> 8) & 1);
    decryptCmcGraphicsInternal(&buf[4 * rpos + 1], &buf[4 * rpos + 2], c[4 * rpos+1], c[4 * rpos+2],
            cmc.type0_t12, cmc.type0_t03, cmc.type1_t12, rpos,
            ((rpos >> 16) ^ cmc.address_16_23_xor2[(rpos >> 8) & 0xff]) & 1);
  }

  // Address xor
  for(auto rpos = 0; rpos < c.size() / 4; rpos++) {
    auto baser = rpos;
    baser ^= key;

    baser ^= cmc.address_8_15_xor1[(baser >> 16) & 0xff] << 8;
    baser ^= cmc.address_8_15_xor2[baser & 0xff] << 8;
    baser ^= cmc.address_16_23_xor1[baser & 0xff] << 16;
    baser ^= cmc.address_16_23_xor2[(baser >> 8) & 0xff] << 16;
    baser ^= cmc.address_0_7_xor[(baser >> 8) & 0xff];

    if(c.size() == 0x3000000) { // special handling for preisle2
      if (rpos < 0x2000000 / 4) baser &= (0x2000000 /4 ) - 1;
      else baser = 0x2000000 / 4 + (baser & ((0x1000000 / 4) - 1));
    }
    else if (c.size() == 0x6000000) { // special handling for kf2k3pcb
      if (rpos < 0x4000000 / 4) baser &= (0x4000000 / 4) - 1;
      else baser = 0x4000000 / 4 + (baser & ((0x1000000 / 4) - 1));
    }
    else baser &= (c.size() / 4) -1; // Clamp to the real rom size

    c[4 * rpos + 0] = buf[4 * baser + 0];
    c[4 * rpos + 1] = buf[4 * baser + 1];
    c[4 * rpos + 2] = buf[4 * baser + 2];
    c[4 * rpos + 3] = buf[4 * baser + 3];
  }
}

auto NeoGeo::decryptCmcGraphicsInternal(u8 *r0, u8 *r1, u8 c0,  u8 c1, u8 *table0hi,u8 *table0lo,
                                        u8 *table1, int base, int invert) -> void {
  uint8_t tmp, xor0, xor1;

  tmp = table1[(base & 0xff) ^ cmc.address_0_7_xor[(base >> 8) & 0xff]];
  xor0 = (table0hi[(base >> 8) & 0xff] & 0xfe) | (tmp & 0x01);
  xor1 = (tmp & 0xfe) | (table0lo[(base >> 8) & 0xff] & 0x01);

  if (invert) {
    *r0 = c1 ^ xor0;
    *r1 = c0 ^ xor1;
    return;
  }

  *r0 = c0 ^ xor0;
  *r1 = c1 ^ xor1;
}

auto NeoGeo::decryptCmcMusic(vector<u8>& m) -> void {
  vector<u8> input = m;

  //checksum of the first 64k of ROM is used as a key
  u16 key = 0;
  for(auto i = 0; i < 0x10000; i++) key += input[i];
  for(auto i = 0; i < input.size(); i++) m[i] = input[decryptCmcMusicDescramble(i, key)];
}

auto NeoGeo::decryptCmcMusicDescramble(u32 address, u16 key) -> u32 {
  const int p1[8][16] = {
    {15,14,10, 7, 1, 2, 3, 8, 0,12,11,13, 6, 9, 5, 4},
    { 7, 1, 8,11,15, 9, 2, 3, 5,13, 4,14,10, 0, 6,12},
    { 8, 6,14, 3,10, 7,15, 1, 4, 0, 2, 5,13,11,12, 9},
    { 2, 8,15, 9, 3, 4,11, 7,13, 6, 0,10, 1,12,14, 5},
    { 1,13, 6,15,14, 3, 8,10, 9, 4, 7,12, 5, 2, 0,11},
    {11,15, 3, 4, 7, 0, 9, 2, 6,14,12, 1, 8, 5,10,13},
    {10, 5,13, 8, 6,15, 1,14,11, 9, 3, 0,12, 7, 4, 2},
    { 9, 3, 7, 0, 2,12, 4,11,14,10, 5, 8,15,13, 1, 6},
  };

  int block = (address >> 16) & 7;
  int aux = address & 0xffff;

  aux ^= BITSWAP16(key,12,0,2,4,8,15,7,13,10,1,3,6,11,9,14,5);
  aux = BITSWAP16(aux,
                    p1[block][15], p1[block][14], p1[block][13], p1[block][12],
                    p1[block][11], p1[block][10], p1[block][ 9], p1[block][ 8],
                    p1[block][ 7], p1[block][ 6], p1[block][ 5], p1[block][ 4],
                    p1[block][ 3], p1[block][ 2], p1[block][ 1], p1[block][ 0]);

  aux ^= m1_address_0_7_xor[(aux >> 8) & 0xff];
  aux ^= m1_address_8_15_xor[aux & 0xff] << 8;
  aux = BITSWAP16(aux, 7,15,14,6,5,13,12,4,11,3,10,2,9,1,8,0);

  return (block << 16) | aux;
}
