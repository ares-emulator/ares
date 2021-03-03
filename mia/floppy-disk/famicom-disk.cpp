auto FamicomDisk::load(string location) -> shared_pointer<vfs::directory> {
  if(directory::exists(location)) {
    auto pak = shared_pointer{new vfs::directory};
    vector<u8> input;
    pak->append("manifest.bml", heuristics(input, location));
    for(auto& filename : directory::files(location, "disk?*.side?*")) {
      pak->append(filename, file::read({location, filename}));
    }
    return pak;
  }

  if(file::exists(location)) {
    auto pak = shared_pointer{new vfs::directory};
    vector<u8> input = FloppyDisk::read(location);

    auto manifest = heuristics(input, location);
    auto document = BML::unserialize(manifest);
    pak->append("manifest.bml", manifest);
    pak->setAttribute("title", document["game/title"].string());

    array_view<u8> view{input};
    if(view.size() % 65500 == 16) view += 16;  //skip iNES / fwNES header
    u32 index = 0;
    while(auto output = transform(view)) {
      string name;
      name.append("disk", (char)('1' + index / 2), ".");
      name.append("side", (char)('A' + index % 2));
      pak->append(name, output);
      view += 65500;
      index++;
    }
    return pak;
  }

  return {};
}

auto FamicomDisk::save(string location, shared_pointer<vfs::directory> pak) -> bool {
  auto fp = pak->read("manifest.bml");
  if(!fp) return false;

  auto manifest = fp->reads();
  auto document = BML::unserialize(manifest);

  return true;
}

auto FamicomDisk::heuristics(vector<u8>& data, string location) -> string {
  string s;
  s += "game\n";
  s +={"  name:  ", Media::name(location), "\n"};
  s +={"  title: ", Media::name(location), "\n"};
  return s;
}

auto FamicomDisk::transform(array_view<u8> input) -> vector<u8> {
  if(input.size() < 65500) return {};

  array_view<u8> data{input.data(), 65500};
  if(data[0x00] != 0x01) return {};
  if(data[0x38] != 0x02) return {};
  if(data[0x3a] != 0x03) return {};
  if(data[0x4a] != 0x04) return {};

  vector<u8> output;
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
    output.append(byte);
  };
  auto flush = [&] {
    hash(0x00);
    hash(0x00);
    output.append(crc16 >> 0);
    output.append(crc16 >> 8);
    crc16 = 0;
  };

  //block 1
  for(u32 n : range(0xe00)) write(0x00);  //pregap
  write(0x80);
  for(u32 n : range(0x38)) write(*data++);
  flush();

  //block 2
  for(u32 n : range(0x80)) write(0x00);  //gap
  write(0x80);
  for(u32 n : range(0x02)) write(*data++);
  flush();

  while(true) {
    if(data[0x00] != 0x03 || data.size() < 0x11) break;
    u16 size = data[0x0d] << 0 | data[0x0e] << 8;
    if(data[0x10] != 0x04 || data.size() < 0x11 + size) break;

    //block 3
    for(u32 n : range(0x80)) write(0x00);  //gap
    write(0x80);
    for(u32 n : range(0x10)) write(*data++);
    flush();

    //block 4
    for(u32 n : range(0x80)) write(0x00);  //gap
    write(0x80);
    for(u32 n : range(1 + size)) write(*data++);
    flush();
  }

  //note: actual maximum capacity of a Famicom Disk is currently unknown
  while(output.size() < 0x12000) output.append(0x00);  //expand if too small
  output.resize(0x12000);  //shrink if too large
  return output;
}
