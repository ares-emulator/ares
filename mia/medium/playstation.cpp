struct PlayStation : CompactDisc {
  auto name() -> string override { return "PlayStation"; }
  auto extensions() -> std::vector<string> override {
#if defined(ARES_ENABLE_CHD)
    return {"cue", "chd", "exe", "ps-exe"};
#else
    return {"cue", "exe", "ps-exe"};
#endif
  }
  auto load(string location) -> LoadResult override;
  auto save(string location) -> bool override;
  auto analyze(string location) -> string;
  auto cdFromExecutable(string location) -> std::vector<u8>;
};

auto PlayStation::load(string location) -> LoadResult {
  if(!inode::exists(location)) return romNotFound;

  this->location = location;
  this->manifest = analyze(location);
  auto document = BML::unserialize(manifest);
  if(!document) return couldNotParseManifest;

  pak = std::make_shared<vfs::directory>();
  pak->setAttribute("title",  document["game/title"].string());
  pak->setAttribute("region", document["game/region"].string());
  pak->setAttribute("audio", (bool)document["game/audio"]);
  pak->append("manifest.bml", manifest);
  if(directory::exists(location)) {
    pak->append("cd.rom", vfs::disk::open({location, "cd.rom"}, vfs::read));
  }
  if(file::exists(location)) {
    if(location.iendsWith(".cue")) {
      pak->append("cd.rom", vfs::cdrom::open(location));
    }
    if(location.iendsWith(".chd")) {
      pak->append("cd.rom", vfs::cdrom::open(location));
    }
    if(location.iendsWith(".exe") || location.iendsWith(".ps-exe")) {
      auto image = cdFromExecutable(location);
      if(image.empty()) return invalidROM;
      pak->append("cd.rom", vfs::memory::open(image));
      pak->setAttribute("executable-disc", true);
    }
  }

  return successful;
}

auto PlayStation::save(string location) -> bool {
  return true;
}

auto PlayStation::analyze(string location) -> string {
  if(location.iendsWith(".cue") || location.iendsWith(".chd")) {
    if(isAudioCd(location)) return CompactDisc::manifestAudio(location);

    std::vector<u8> sector;
    sector = readDataSector(location, 4);

    if(sector.empty()) return CompactDisc::manifestAudio(location);

    string text;
    text.resize(sector.size());
    memory::copy(text.get(), sector.data(), sector.size());

    string region;
    if(text.find("Sony Computer Entertainment Inc."      )) region = "NTSC-J";
    if(text.find("Sony Computer Entertainment Amer"      )) region = "NTSC-U";
    if(text.find("Sony Computer Entertainment of America")) region = "NTSC-U";
    if(text.find("Sony Computer Entertainment Euro"      )) region = "PAL";
    if(!region) {
      //A valid license string is only required for NTSC-J hardware
      region = "NTSC-U, PAL";
    }

    string s;
    s += "game\n";
    s +={"  name:   ", Medium::name(location), "\n"};
    s +={"  title:  ", Medium::name(location), "\n"};
    s +={"  region: ", region, "\n"};
    return s;
  }

  if(location.iendsWith(".exe") || location.iendsWith(".ps-exe")) {
    auto exe = file::read(location);
    if(exe.size() < 2048) return {};
    if(memory::compare(exe.data(), "PS-X EXE", 8)) return {};

    string s;
    s += "game\n";
    s +={"  name:   ", Medium::name(location), "\n"};
    s +={"  title:  ", Medium::name(location), "\n"};
    s +={"  region: ", "NTSC-U, PAL, NTSC-J", "\n"};

    return s;
  }

  return {};
}

auto PlayStation::cdFromExecutable(string location) -> std::vector<u8> {
  auto exe = file::read(location);
  if(exe.size() < 2048 || memory::compare(exe.data(), "PS-X EXE", 8)) return {};
  auto word = [&](u32 offset) -> u32 {
    return (u32)exe[offset] | (u32)exe[offset+1] << 8
      | (u32)exe[offset+2] << 16 | (u32)exe[offset+3] << 24;
  };
  if(word(0x1c) > exe.size() - 2048 || exe.size() > 2_MiB + 2048) return {};
  u32 blocks = (exe.size() + 2047) / 2048;
  u32 sectors = max(750u, 22 + blocks);
  std::vector<u8> iso(sectors * 2048);
  auto le16 = [&](u32 at, u32 value) {
    iso[at] = value; iso[at+1] = value >> 8;
  };
  auto be16 = [&](u32 at, u32 value) {
    iso[at] = value >> 8; iso[at+1] = value;
  };
  auto le32 = [&](u32 at, u32 value) {
    for(u32 n : range(4)) iso[at+n] = value >> (8*n);
  };
  auto be32 = [&](u32 at, u32 value) {
    for(u32 n : range(4)) iso[at+n] = value >> (24-8*n);
  };
  auto both16 = [&](u32 at, u32 value) { le16(at,value); be16(at+2,value); };
  auto both32 = [&](u32 at, u32 value) { le32(at,value); be32(at+4,value); };
  auto text = [&](u32 at, const string& value) {
    memory::copy(iso.data()+at, value.data(), value.size());
  };
  auto record = [&](u32 at, const string& name, u32 lba, u32 size, u8 flags) -> u32 {
    u32 length = (33 + name.size() + 1) & ~1;
    iso[at] = length;
    both32(at+2,lba); both32(at+10,size);
    iso[at+18] = 100; iso[at+19] = 1; iso[at+20] = 1;
    iso[at+25] = flags; both16(at+28,1);
    iso[at+32] = name.size(); text(at+33,name);
    return length;
  };
  text(4*2048,"          Licensed  by          ");
  text(4*2048+32,"Sony Computer Entertainment Amer  ica ");
  memory::copy(iso.data()+5*2048,Resource::PlayStation::DiscLogo,0x3278);
  memory::fill<u8>(iso.data()+5*2048+0x3278,0x588,0xff);
  u32 v = 16*2048;
  iso[v] = 1; text(v+1,"CD001"); iso[v+6] = 1;
  for(u32 n : range(64)) iso[v+8+n] = ' ';
  text(v+8,"PLAYSTATION"); text(v+40,"ARES_EXE");
  both32(v+80,sectors); both16(v+120,1); both16(v+124,1);
  both16(v+128,2048); both32(v+132,10);
  le32(v+140,18); be32(v+148,19);
  string dot; dot.append((char)0);
  string parent; parent.append((char)1);
  record(v+156,dot,20,2048,2); iso[v+881] = 1;
  text(v+1024,"CD-XA001");
  iso[17*2048] = 255; text(17*2048+1,"CD001"); iso[17*2048+6] = 1;
  iso[18*2048] = iso[19*2048] = 1;
  le32(18*2048+2,20); le16(18*2048+6,1);
  be32(19*2048+2,20); be16(19*2048+6,1);
  string cnf = "BOOT = cdrom:\\PSX.EXE;1\r\nTCB = 4\r\nEVENT = 10\r\nSTACK = 801FFF00\r\n";
  u32 directory = 20*2048;
  directory += record(directory,dot,20,2048,2);
  directory += record(directory,parent,20,2048,2);
  directory += record(directory,"SYSTEM.CNF;1",21,cnf.size(),0);
  record(directory,"PSX.EXE;1",22,exe.size(),0);
  text(21*2048,cnf);
  memory::copy(iso.data()+22*2048,exe.data(),exe.size());

  CD::Session session;
  session.leadIn.lba = -(CD::LeadInSectors + CD::Track1Pregap);
  session.leadIn.end = -CD::Track1Pregap - 1;
  session.firstTrack = session.lastTrack = 1;
  auto& track = session.tracks[1];
  track.control = 4; track.firstIndex = 0; track.lastIndex = 1;
  track.indices[0].lba = -CD::Track1Pregap; track.indices[0].end = -1;
  track.indices[1].lba = 0; track.indices[1].end = sectors-1;
  session.leadOut.lba = sectors;
  session.leadOut.end = sectors + CD::LeadOutSectors - 1;
  u32 total = CD::LeadInSectors + CD::Track1Pregap + sectors + CD::LeadOutSectors;
  auto subchannel = session.encode(total);
  std::vector<u8> image((u64)total*2448);
  for(u32 sector : range(total)) {
    memory::copy(image.data()+(u64)sector*2448+2352,subchannel.data()+sector*96,96);
  }
  for(u32 lba : range(sectors)) {
    auto* raw = image.data()+(u64)(CD::LeadInSectors+CD::Track1Pregap+lba)*2448;
    for(u32 n : range(10)) raw[1+n] = 255;
    auto msf = CD::MSF::fromLBA(lba);
    raw[12] = BCD::encode(msf.minute); raw[13] = BCD::encode(msf.second);
    raw[14] = BCD::encode(msf.frame); raw[15] = 2;
    raw[18] = raw[22] = 8;
    memory::copy(raw+24,iso.data()+lba*2048,2048);
    CD::EDC::create({raw+16,2056},{raw+2072,4});
    u8 header[4]; memory::copy(header,raw+12,4);
    memory::fill<u8>(raw+12,4,0);
    CD::RSPC::encodeMode1({raw,2352});
    memory::copy(raw+12,header,4);
  }
  return image;
}
