auto FamicomDisk::export(string location) -> vector<uint8_t> {
  vector<uint8_t> data;
  for(auto& filename : directory::files(location, "disk?*.side?*")) {
    append(data, {location, filename});
  }
  return data;
}

auto FamicomDisk::heuristics(vector<uint8_t>& data, string location) -> string {
  string s;
  s += "game\n";
  s +={"  name:  ", Media::name(location), "\n"};
  s +={"  label: ", Media::name(location), "\n"};
  return s;
}

auto FamicomDisk::import(string location) -> string {
  auto input = Media::read(location);
  if(input.size() < 65500) return "disk image is too small";

  location = {pathname, Location::prefix(location), "/"};
  if(!directory::create(location)) return "output directory not writable";

  if(input.size() % 65500 == 16) {
    //iNES and fwNES headers are unnecessary
    memory::move(&input[0], &input[16], input.size() - 16);
    input.resize(input.size() - 16);
  }

  array_view<uint8_t> view = input;
  uint disk = 0, side = 0;
  while(auto output = transform(view)) {
    string name{"disk", 1 + disk, ".", "side", !side ? "A" : "B"};
    file::write({location, name}, output);

    side ^= 1;
    if(!side) disk++;
    view += 65500;
  }

  return {};
}

auto FamicomDisk::transform(array_view<uint8_t> input) -> vector<uint8_t> {
  if(input.size() < 65500) return {};

  array_view<uint8_t> data{input.data(), 65500};
  if(data[0x00] != 0x01) return {};
  if(data[0x38] != 0x02) return {};
  if(data[0x3a] != 0x03) return {};
  if(data[0x4a] != 0x04) return {};

  vector<uint8_t> output;
  uint16_t crc16 = 0;
  auto hash = [&](uint8_t byte) {
    for(uint bit : range(8)) {
      bool carry = crc16 & 1;
      crc16 = crc16 >> 1 | bool(byte & 1 << bit) << 15;
      if(carry) crc16 ^= 0x8408;
    }
  };
  auto write = [&](uint8_t byte) {
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
  for(uint n : range(0xe00)) write(0x00);  //pregap
  write(0x80);
  for(uint n : range(0x38)) write(*data++);
  flush();

  //block 2
  for(uint n : range(0x80)) write(0x00);  //gap
  write(0x80);
  for(uint n : range(0x02)) write(*data++);
  flush();

  while(true) {
    if(data[0x00] != 0x03 || data.size() < 0x11) break;
    uint16_t size = data[0x0d] << 0 | data[0x0e] << 8;
    if(data[0x10] != 0x04 || data.size() < 0x11 + size) break;

    //block 3
    for(uint n : range(0x80)) write(0x00);  //gap
    write(0x80);
    for(uint n : range(0x10)) write(*data++);
    flush();

    //block 4
    for(uint n : range(0x80)) write(0x00);  //gap
    write(0x80);
    for(uint n : range(1 + size)) write(*data++);
    flush();
  }

  //note: actual maximum capacity of a Famicom Disk is currently unknown
  while(output.size() < 0x12000) output.append(0x00);  //expand if too small
  output.resize(0x12000);  //shrink if too large
  return output;
}
