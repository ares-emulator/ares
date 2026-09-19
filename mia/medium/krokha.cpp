//a Krokha cart is two separate mask ROMs, so a bare dump of one of them is not
//enough to run anything. no container format exists for this machine, so this
//defines one: a 16 byte header naming the size of each chip, followed by the
//program image and then the character generator image.
//
//  offset  size  contents
//  0x00    6     "KROKHA"
//  0x06    1     format version, currently 1
//  0x07    1     reserved, must be zero
//  0x08    4     program.rom size, little endian
//  0x0c    4     character.rom size, little endian
//
//loose dumps are still accepted two other ways: a directory holding
//program.rom and character.rom, or a headerless 10240 byte file, which is the
//only known cartridge and splits 8192/2048.

struct Krokha : Cartridge {
  auto name() -> string override { return "Krokha"; }
  auto extensions() -> std::vector<string> override { return {"krk"}; }
  auto load(string location) -> LoadResult override;
  auto save(string location) -> bool override;
  auto analyze(std::vector<u8>& rom, std::vector<u8>& chr) -> string;

  static constexpr u32 headerSize = 16;
};

auto Krokha::load(string location) -> LoadResult {
  std::vector<u8> rom;
  std::vector<u8> chr;

  if(directory::exists(location)) {
    append(rom, {location, "program.rom"});
    append(chr, {location, "character.rom"});
  } else if(file::exists(location)) {
    auto image = Cartridge::read(location);
    if(image.size() >= headerSize && !memory::compare(image.data(), "KROKHA", 6)) {
      if(image[6] != 1) return invalidROM;
      u32 programSize   = image[ 8] << 0 | image[ 9] << 8 | image[10] << 16 | image[11] << 24;
      u32 characterSize = image[12] << 0 | image[13] << 8 | image[14] << 16 | image[15] << 24;
      if(headerSize + (u64)programSize + characterSize > image.size()) return invalidROM;
      auto data = image.begin() + headerSize;
      rom.assign(data, data + programSize);
      chr.assign(data + programSize, data + programSize + characterSize);
    } else if(image.size() == 10_KiB) {
      rom.assign(image.begin(), image.begin() + 8_KiB);
      chr.assign(image.begin() + 8_KiB, image.end());
    } else {
      return invalidROM;
    }
  }

  if(rom.empty() || chr.empty()) return romNotFound;

  this->sha256   = Hash::SHA256(rom).digest();
  this->location = location;
  this->manifest = analyze(rom, chr);
  auto document = BML::unserialize(manifest);
  if(!document) return couldNotParseManifest;

  pak = std::make_shared<vfs::directory>();
  pak->setAttribute("title", document["game/title"].string());
  pak->append("manifest.bml",  manifest);
  pak->append("program.rom",   rom);
  pak->append("character.rom", chr);

  return successful;
}

auto Krokha::save(string location) -> bool {
  auto document = BML::unserialize(manifest);

  return true;
}

auto Krokha::analyze(std::vector<u8>& rom, std::vector<u8>& chr) -> string {
  string s;
  s += "game\n";
  s +={"  name:   ", Medium::name(location), "\n"};
  s +={"  title:  ", Medium::name(location), "\n"};
  s +={"  sha256: ", sha256, "\n"};
  s += "  board\n";
  s += "    memory\n";
  s += "      type: ROM\n";
  s +={"      size: 0x", hex(rom.size()), "\n"};
  s += "      content: Program\n";
  s += "    memory\n";
  s += "      type: ROM\n";
  s +={"      size: 0x", hex(chr.size()), "\n"};
  s += "      content: Character\n";
  return s;
}
