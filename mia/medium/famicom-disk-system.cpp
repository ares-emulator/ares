struct FamicomDiskSystem : FloppyDisk {
  auto name() -> string override { return "Famicom Disk System"; }
  auto extensions() -> std::vector<string> override { return {"fds"}; }
  auto load(string location) -> LoadResult override;
  auto save(string location) -> bool override;
  auto analyze() -> string;
  auto transform(array_view<u8> input) -> std::vector<u8>;
};

auto FamicomDiskSystem::load(string location) -> LoadResult {
  if(directory::exists(location)) {
    this->location = location;
    this->manifest = analyze();

    pak = new vfs::directory;
    pak->setAttribute("title", Medium::name(location));
    pak->append("manifest.bml", manifest);
    for(auto& filename : directory::files(location, "disk?*.side?*")) {
      auto mem = file::read({location, filename});
      pak->append(filename, mem);
    }
  }

  if(file::exists(location)) {
    this->location = location;
    this->manifest = analyze();

    pak = new vfs::directory;
    pak->setAttribute("title", Medium::name(location));
    pak->append("manifest.bml", manifest);

    std::vector<u8> input = FloppyDisk::read(location);
    array_view<u8> view{input};
    if(view.size() % 65500 == 16) view += 16;  //skip iNES / fwNES header
    u32 index = 0;

    auto output = transform(view);
    do {
      string name;
      name.append("disk", (char)('1' + index / 2), ".");
      name.append("side", (char)('A' + index % 2));
      pak->append(name, output);
      view += 65500;
      index++;
      output = transform(view);
    } while (!output.empty());
  }

  if(!pak) return romNotFound;

  Pak::load("disk1.sideA", ".d1a");
  Pak::load("disk1.sideB", ".d1b");
  Pak::load("disk2.sideA", ".d2a");
  Pak::load("disk2.sideB", ".d2b");

  return successful;
}

auto FamicomDiskSystem::save(string location) -> bool {
  auto document = BML::unserialize(manifest);

  Pak::save("disk1.sideA", ".d1a");
  Pak::save("disk1.sideB", ".d1b");
  Pak::save("disk2.sideA", ".d2a");
  Pak::save("disk2.sideB", ".d2b");

  return true;
}

auto FamicomDiskSystem::analyze() -> string {
  string s;
  s += "game\n";
  s +={"  name:  ", Medium::name(location), "\n"};
  s +={"  title: ", Medium::name(location), "\n"};
  return s;
}

auto FamicomDiskSystem::transform(array_view<u8> input) -> std::vector<u8> {
  if(input.size() < 65500) return {};

  array_view<u8> data{input.data(), 65500};
  if(data[0x00] != 0x01) return {};
  if(data[0x38] != 0x02) return {};
  if(data[0x3a] != 0x03) return {};
  if(data[0x4a] != 0x04) return {};

  std::vector<u8> output;
  u16 crc16 = 0;
  auto hash = [&](u8 byte) {
    for(u32 bit : range(8)) {
      bool carry = crc16 & 1;
      crc16 = crc16 >> 1 | bool(byte & 1 << bit) << 15;
      if(carry) crc16 ^= 0x8408;
    }
  };
  auto write = [&](u8 byte) {
    hash(byte);
    output.push_back(byte);
  };
  auto flush = [&] {
    hash(0x00);
    hash(0x00);
    output.push_back(crc16 >> 0);
    output.push_back(crc16 >> 8);
    crc16 = 0;
  };

  //block 1
  for(u32 n : range(0xe00)) write(0x00);  //pregap
  write(0x80);
  for(u32 n : range(0x38)) write(data.read());
  flush();

  //block 2
  for(u32 n : range(0x80)) write(0x00);  //gap
  write(0x80);
  for(u32 n : range(0x02)) write(data.read());
  flush();

  while(true) {
    if(data[0x00] != 0x03 || data.size() < 0x11) break;
    u16 size = data[0x0d] << 0 | data[0x0e] << 8;
    if(data[0x10] != 0x04 || data.size() < 0x11 + size) break;

    //block 3
    for(u32 n : range(0x80)) write(0x00);  //gap
    write(0x80);
    for(u32 n : range(0x10)) write(data.read());
    flush();

    //block 4
    for(u32 n : range(0x80)) write(0x00);  //gap
    write(0x80);
    for(u32 n : range(1 + size)) write(data.read());
    flush();
  }

  //note: actual maximum capacity of a Famicom Disk is currently unknown
  while(output.size() < 0x12000) output.push_back(0x00);  //expand if too small
  output.resize(0x12000);  //shrink if too large
  return output;
}
