struct SuperFamicom : Cartridge {
  auto name() -> string override { return "Super Famicom"; }
  auto extensions() -> vector<string> override { return {"sfc", "smc", "swc", "fig"}; }
  auto load(string location) -> bool override;
  auto save(string location) -> bool override;
  auto analyze(vector<u8>& rom) -> string;

protected:
  auto region() const -> string;
  auto videoRegion() const -> string;
  auto revision() const -> string;
  auto board() const -> string;
  auto label() const -> string;
  auto serial() const -> string;
  auto romSize() const -> u32;
  auto programRomSize() const -> u32;
  auto dataRomSize() const -> u32;
  auto expansionRomSize() const -> u32;
  auto firmwareRomSize() const -> u32;
  auto ramSize() const -> u32;
  auto expansionRamSize() const -> u32;
  auto nonVolatile() const -> bool;

  auto size() const -> u32 { return data.size(); }
  auto scoreHeader(u32 address) -> u32;
  auto firmwareARM() const -> string;
  auto firmwareEXNEC() const -> string;
  auto firmwareGB() const -> string;
  auto firmwareHITACHI() const -> string;
  auto firmwareNEC() const -> string;

  array_view<u8> data;
  u32 headerAddress = 0;
};

auto SuperFamicom::load(string location) -> bool {
  vector<u8> rom;
  string directory = location;
  bool local_firmware = false;
  bool folder = false;
  
  auto append_missing_firmware = [&](auto document)
  {
    if(auto identifier = document["game/board/memory/identifier"]) {
      if(!firmwareRomSize() && !local_firmware) {
        auto id = identifier.string();
        array_view<u8> view;
        if(id == "Cx4"  ) view = Resource::SuperFamicom::Cx4;
        if(id == "DSP1" ) view = Resource::SuperFamicom::DSP1;
        if(id == "DSP1B") view = Resource::SuperFamicom::DSP1B;
        if(id == "DSP2" ) view = Resource::SuperFamicom::DSP2;
        if(id == "DSP3" ) view = Resource::SuperFamicom::DSP3;
        if(id == "DSP4" ) view = Resource::SuperFamicom::DSP4;
        if(id == "SGB1" ) view = Resource::SuperFamicom::SGB1;
        if(id == "SGB2" ) view = Resource::SuperFamicom::SGB2;
        if(id == "ST010") view = Resource::SuperFamicom::ST010;
        if(id == "ST011") view = Resource::SuperFamicom::ST011;
        if(id == "ST018") view = Resource::SuperFamicom::ST018;
        while(view) rom.append(*view++);
      }
	}
  };
  
  if(directory::exists(location)) {
    auto files = directory::files(location, "*.rom");
    append(rom, {location, "program.rom"  });
    append(rom, {location, "data.rom"     });
    append(rom, {location, "expansion.rom"});
    for(auto& file : files.match("slot-*.rom"   )) { append(rom, {location, file});                        }
    for(auto& file : files.match("*.program.rom")) { append(rom, {location, file}); local_firmware = true; }
    for(auto& file : files.match("*.data.rom"   )) { append(rom, {location, file}); local_firmware = true; }
    for(auto& file : files.match("*.boot.rom"   )) { append(rom, {location, file});                        }
	folder = true;
  } else if(rom = Medium::read(location)) {
    directory = Location::dir(location);
  }
  
  if(!rom) return false;
  
  //append firmware to the ROM if it is missing
  auto tmp_manifest = analyze(rom);
  auto document = BML::unserialize(tmp_manifest);
  append_missing_firmware(document);
  
  this->sha256   = Hash::SHA256(rom).digest();
  this->location = location;
  this->manifest = Medium::manifestDatabase(sha256);
  
  if(!manifest) {
    auto local_manifest = location.replace({".", location.split(".").last()}, ".bml");
    if (folder)
      local_manifest = directory.append("manifest.bml");
    if(file::exists(local_manifest)) {
      manifest = file::read(local_manifest);
    }
  }  
  
  document = BML::unserialize(manifest);
  append_missing_firmware(document); // if auto-detect failed, use chips from the manifest
  
  if(!manifest) manifest = analyze(rom);
  document = BML::unserialize(manifest);
  if(!document) return false;

  pak = new vfs::directory;
  pak->setAttribute("title", document["game/title"].string());
  pak->setAttribute("board", document["game/board"].string());
  if(auto node = document["game/board/oscillator/frequency"]) {
    pak->setAttribute("oscillator", node.natural());
  }
  auto region = document["game/region"].string();

  if(region.endsWith("SGB")) {
    region = "NTSC, PAL";
  } else if(region.endsWith("BRA")
  || region.endsWith("CAN")
  || region.endsWith("HKG")
  || region.endsWith("JPN")
  || region.endsWith("KOR")
  || region.endsWith("LTN")
  || region.endsWith("ROC")
  || region.endsWith("USA")
  || region.beginsWith("SHVC-")
  || region == "NTSC"
  ) {
    region = "NTSC";
  } else {
    region = "PAL";
  }

  pak->setAttribute("region", region);
  pak->append("manifest.bml", manifest);

  //find all msu1 files
  auto files = directory::files(directory, "*.msu");
  for(auto _file : directory::files(directory, "*-*.pcm")) {
    files.append(_file);
  }
  for(auto _file : directory::files(directory, "msu1.data.rom")) {
    files.append(_file);
  }

  for(auto& _file : files) {
    //add msu-1 rom
    if(_file.imatch("*.msu") || _file == "msu1.data.rom") {
      pak->append("msu1.data.rom", file::read({directory, "/", _file}));
    }

    //add msu-1 audio tracks
    if(_file.imatch("*-*.pcm")) {
      auto track = _file.split("-").last().replace(".pcm", "").integer();
      pak->append({"msu1.track-", track,".pcm"}, file::read({directory, "/", _file}));
    }
  }

  array_view<u8> view{rom};
  for(auto node : document.find("game/board/memory(type=ROM)")) {
    string name;
    if(auto architecture = node["architecture"].string()) name.append(architecture.downcase(), ".");
    name.append(node["content"].string().downcase(), ".rom");
    u32 size = node["size"].natural();
    if(view.size() < size) break;  //missing firmware
    pak->append(name, {view.data(), size});
    view += size;
  }

  if(auto node = document["game/board/memory(type=RAM,content=Save)"]) {
    Medium::load(node, ".ram");
  }
  if(auto node = document["game/board/memory(type=RAM,content=Internal)"]) {
    Medium::load(node, ".iram");
  }
  if(auto node = document["game/board/memory(type=RAM,content=Download)"]) {
    Medium::load(node, ".bsx");
  }
  if(auto node = document["game/board/memory(type=RTC,content=Time)"]) {
    Medium::load(node, ".rtc");
  }
  if(auto node = document["game/board/memory(type=RAM,content=Data)"]) {
    Medium::load(node, ".dram");
  }

  return true;
}

auto SuperFamicom::save(string location) -> bool {
  auto document = BML::unserialize(manifest);

  if(auto node = document["game/board/memory(type=RAM,content=Save)"]) {
    Medium::save(node, ".ram");
  }
  if(auto node = document["game/board/memory(type=RAM,content=Internal)"]) {
    Medium::save(node, ".iram");
  }
  if(auto node = document["game/board/memory(type=RAM,content=Download)"]) {
    Medium::save(node, ".bsx");
  }
  if(auto node = document["game/board/memory(type=RTC,content=Time)"]) {
    Medium::save(node, ".rtc");
  }
  if(auto node = document["game/board/memory(type=RAM,content=Data)"]) {
    Medium::save(node, ".dram");
  }

  return true;
}

auto SuperFamicom::analyze(vector<u8>& rom) -> string {
  if((rom.size() & 0x7fff) == 512) {
    //remove header if present
    memory::move(&rom[0], &rom[512], rom.size() - 512);
    rom.resize(rom.size() - 512);
  }

  //ignore images too small to be valid
  if(rom.size() < 0x8000) {
    print("[mia] Loading rom failed. Minimum expected rom size is 32768 (0x8000) bytes. Rom size: ", rom.size(), " (0x", hex(rom.size()), ") bytes.\n");
    return {};
  }
  data = rom;

  u32 LoROM   = scoreHeader(  0x7fb0);
  u32 HiROM   = scoreHeader(  0xffb0);
  u32 ExLoROM = scoreHeader(0x407fb0);
  u32 ExHiROM = scoreHeader(0x40ffb0);
  if(ExLoROM) ExLoROM += 4;
  if(ExHiROM) ExHiROM += 4;

  if(LoROM >= HiROM && LoROM >= ExLoROM && LoROM >= ExHiROM) headerAddress = 0x7fb0;
  else if(HiROM >= ExLoROM && HiROM >= ExHiROM) headerAddress = 0xffb0;
  else if(ExLoROM >= ExHiROM) headerAddress = 0x407fb0;
  else headerAddress = 0x40ffb0;

  string s;
  auto& output = s;
  s += "game\n";
  s +={"  sha256:   ", sha256, "\n"};
  s +={"  name:     ", Medium::name(location), "\n"};
  s +={"  title:    ", Medium::name(location), "\n"};
  s +={"  label:    ", label(), "\n"};
  s +={"  region:   ", region(), "\n"};
  s +={"  revision: ", revision(), "\n"};
  s +={"  board:    ", board(), "\n"};

  auto board = this->board().trimRight("#A", 1L).split("-");

  if(auto size = romSize()) {
    if(board(0) == "SPC7110" && size > 0x100000) {
      s += "    memory\n";
      s += "      type: ROM\n";
      s += "      size: 0x100000\n";
      s += "      content: Program\n";
      s += "    memory\n";
      s += "      type: ROM\n";
      s +={"      size: 0x", hex(size - 0x100000), "\n"};
      s += "      content: Data\n";
    } else if(board(0) == "EXSPC7110" && size == 0x700000) {
      //Tengai Maykou Zero (fan translation)
      s += "    memory\n";
      s += "      type: ROM\n";
      s += "      size: 0x100000\n";
      s += "      content: Program\n";
      s += "    memory\n";
      s += "      type: ROM\n";
      s += "      size: 0x500000\n";
      s += "      content: Data\n";
      s += "    memory\n";
      s += "      type: ROM\n";
      s += "      size: 0x100000\n";
      s += "      content: Expansion\n";
    } else {
      s += "    memory\n";
      s += "      type: ROM\n";
      s +={"      size: 0x", hex(size), "\n"};
      s += "      content: Program\n";
    }
  }

  if(auto size = ramSize()) {
    s += "    memory\n";
    s += "      type: RAM\n";
    s +={"      size: 0x", hex(size), "\n"};
    s += "      content: Save\n";
  }

  if(auto size = expansionRamSize()) {
    s += "    memory\n";
    s += "      type: RAM\n";
    s +={"      size: 0x", hex(size), "\n"};
    s += "      content: Save\n";
  }

  if(board(0) == "ARM") {
    s += "    memory\n";
    s += "      type: ROM\n";
    s += "      size: 0x20000\n";
    s += "      content: Program\n";
    s += "      manufacturer: SETA\n";
    s += "      architecture: ARM6\n";
    s +={"      identifier: ", firmwareARM(), "\n"};
    s += "    memory\n";
    s += "      type: ROM\n";
    s += "      size: 0x8000\n";
    s += "      content: Data\n";
    s += "      manufacturer: SETA\n";
    s += "      architecture: ARM6\n";
    s +={"      identifier: ", firmwareARM(), "\n"};
    s += "    memory\n";
    s += "      type: RAM\n";
    s += "      size: 0x4000\n";
    s += "      content: Data\n";
    s += "      manufacturer: SETA\n";
    s += "      architecture: ARM6\n";
    s +={"      identifier: ", firmwareARM(), "\n"};
    s += "      volatile\n";
    s += "    oscillator\n";
    s += "      frequency: 21440000\n";
  } else if(board(0) == "BS" && board(1) == "MCC") {
    s += "    memory\n";
    s += "      type: RAM\n";
    s += "      size: 0x80000\n";
    s += "      content: Download\n";
  } else if(board(0) == "EXNEC") {
    s += "    memory\n";
    s += "      type: ROM\n";
    s += "      size: 0xc000\n";
    s += "      content: Program\n";
    s += "      manufacturer: NEC\n";
    s += "      architecture: uPD96050\n";
    s +={"      identifier: ", firmwareEXNEC(), "\n"};
    s += "    memory\n";
    s += "      type: ROM\n";
    s += "      size: 0x1000\n";
    s += "      content: Data\n";
    s += "      manufacturer: NEC\n";
    s += "      architecture: uPD96050\n";
    s +={"      identifier: ", firmwareEXNEC(), "\n"};
    s += "    memory\n";
    s += "      type: RAM\n";
    s += "      size: 0x1000\n";
    s += "      content: Data\n";
    s += "      manufacturer: NEC\n";
    s += "      architecture: uPD96050\n";
    s +={"      identifier: ", firmwareEXNEC(), "\n"};
    s += "    oscillator\n";
    s +={"      frequency: ", firmwareEXNEC() == "ST010" ? 11000000 : 15000000, "\n"};
  } else if(board(0) == "GB") {
    s += "    memory\n";
    s += "      type: ROM\n";
    s += "      size: 0x100\n";
    s += "      content: Boot\n";
    s += "      manufacturer: Nintendo\n";
    s += "      architecture: SM83\n";
    s +={"      identifier: ", firmwareGB(), "\n"};
    if(firmwareGB() == "SGB2") {
      s += "    oscillator\n";
      s += "      frequency: 20971520\n";
    }
  } else if(board(0) == "GSU") {
  //todo: MARIO CHIP 1 uses CPU oscillator
    s += "    oscillator\n";
    s += "      frequency: 21440000\n";
  } else if(board(0) == "HITACHI") {
    s += "    memory\n";
    s += "      type: ROM\n";
    s += "      size: 0xc00\n";
    s += "      content: Data\n";
    s += "      manufacturer: Hitachi\n";
    s += "      architecture: HG51BS169\n";
    s +={"      identifier: ", firmwareHITACHI(), "\n"};
    s += "    memory\n";
    s += "      type: RAM\n";
    s += "      size: 0xc00\n";
    s += "      content: Data\n";
    s += "      manufacturer: Hitachi\n";
    s += "      architecture: HG51BS169\n";
    s +={"      identifier: ", firmwareHITACHI(), "\n"};
    s += "      volatile\n";
    s += "    oscillator\n";
    s += "      frequency: 20000000\n";
  } else if(board(0) == "NEC") {
    s += "    memory\n";
    s += "      type: ROM\n";
    s += "      size: 0x1800\n";
    s += "      content: Program\n";
    s += "      manufacturer: NEC\n";
    s += "      architecture: uPD7725\n";
    s +={"      identifier: ", firmwareNEC(), "\n"};
    s += "    memory\n";
    s += "      type: ROM\n";
    s += "      size: 0x800\n";
    s += "      content: Data\n";
    s += "      manufacturer: NEC\n";
    s += "      architecture: uPD7725\n";
    s +={"      identifier: ", firmwareNEC(), "\n"};
    s += "    memory\n";
    s += "      type: RAM\n";
    s += "      size: 0x200\n";
    s += "      content: Data\n";
    s += "      manufacturer: NEC\n";
    s += "      architecture: uPD7725\n";
    s +={"      identifier: ", firmwareNEC(), "\n"};
    s += "      volatile\n";
    s += "    oscillator\n";
    s += "      frequency: 7600000\n";
  } else if(board(0) == "SA1" || board(1) == "SA1") {  //SA1-* or BS-SA1-*
    s += "    memory\n";
    s += "      type: RAM\n";
    s += "      size: 0x800\n";
    s += "      content: Internal\n";
    s += "      volatile\n";
  }

  if(board.right() == "EPSONRTC") {
    s += "    memory\n";
    s += "      type: RTC\n";
    s += "      size: 0x10\n";
    s += "      content: Time\n";
    s += "      manufacturer: Epson\n";
  } else if(board.right() == "SHARPRTC") {
    s += "    memory\n";
    s += "      type: RTC\n";
    s += "      size: 0x10\n";
    s += "      content: Time\n";
    s += "      manufacturer: Sharp\n";
  }

  return output;
}

auto SuperFamicom::region() const -> string {
  //Unlicensed software (homebrew, ROM hacks, etc) often change the standard region code,
  //and then neglect to change the extended header region code. Thanks to that, we can't
  //decode and display the full game serial + region code.
  return videoRegion();

  string region;

  char A = data[headerAddress + 0x02];  //game type
  char B = data[headerAddress + 0x03];  //game code
  char C = data[headerAddress + 0x04];  //game code
  char D = data[headerAddress + 0x05];  //region code (new; sometimes ambiguous)
  auto E = data[headerAddress + 0x29];  //region code (old)

  auto valid = [](char n) { return (n >= '0' && n <= '9') || (n >= 'A' && n <= 'Z'); };
  if(data[headerAddress + 0x2a] == 0x33 && valid(A) && valid(B) & valid(C) & valid(D)) {
    string code{A, B, C, D};
    if(D == 'B') region = {"SNS-",  code, "-BRA"};
    if(D == 'C') region = {"SNSN-", code, "-ROC"};
    if(D == 'D') region = {"SNSP-", code, "-NOE"};
    if(D == 'E') region = {"SNS-",  code, "-USA"};
    if(D == 'F') region = {"SNSP-", code, "-FRA"};
    if(D == 'H') region = {"SNSP-", code, "-HOL"};
    if(D == 'I') region = {"SNSP-", code, "-ITA"};
    if(D == 'J') region = {"SHVC-", code, "-JPN"};
    if(D == 'K') region = {"SNSN-", code, "-KOR"};
    if(D == 'N') region = {"SNS-",  code, "-CAN"};
    if(D == 'P') region = {"SNSP-", code, "-EUR"};
    if(D == 'S') region = {"SNSP-", code, "-ESP"};
    if(D == 'U') region = {"SNSP-", code, "-AUS"};
    if(D == 'W') region = {"SNSP-", code, "-SCN"};
  }

  if(!region) {
    if(E == 0x00) region = {"JPN"};
    if(E == 0x01) region = {"USA"};
    if(E == 0x02) region = {"EUR"};
    if(E == 0x03) region = {"SCN"};
    if(E == 0x06) region = {"FRA"};
    if(E == 0x07) region = {"HOL"};
    if(E == 0x08) region = {"ESP"};
    if(E == 0x09) region = {"NOE"};
    if(E == 0x0a) region = {"ITA"};
    if(E == 0x0b) region = {"ROC"};
    if(E == 0x0d) region = {"KOR"};
    if(E == 0x0f) region = {"CAN"};
    if(E == 0x10) region = {"BRA"};
    if(E == 0x11) region = {"AUS"};
  }

  return region ? region : "NTSC"_s;
}

auto SuperFamicom::videoRegion() const -> string {
  auto region = data[headerAddress + 0x29];
  if(region == 0x00) return "NTSC";  //JPN
  if(region == 0x01) return "NTSC";  //USA
  if(region == 0x0b) return "NTSC";  //ROC
  if(region == 0x0d) return "NTSC";  //KOR
  if(region == 0x0f) return "NTSC";  //CAN
  if(region == 0x10) return "NTSC";  //BRA
  return "PAL";
}

auto SuperFamicom::revision() const -> string {
  string revision;

  char A = data[headerAddress + 0x02];  //game type
  char B = data[headerAddress + 0x03];  //game code
  char C = data[headerAddress + 0x04];  //game code
  char D = data[headerAddress + 0x05];  //region code (new; sometimes ambiguous)
  auto E = data[headerAddress + 0x29];  //region code (old)
  auto F = data[headerAddress + 0x2b];  //revision code

  auto valid = [](char n) { return (n >= '0' && n <= '9') || (n >= 'A' && n <= 'Z'); };
  if(data[headerAddress + 0x2a] == 0x33 && valid(A) && valid(B) & valid(C) & valid(D)) {
    string code{A, B, C, D};
    if(D == 'B') revision = {"SNS-",  code, "-", F};
    if(D == 'C') revision = {"SNSN-", code, "-", F};
    if(D == 'D') revision = {"SNSP-", code, "-", F};
    if(D == 'E') revision = {"SNS-",  code, "-", F};
    if(D == 'F') revision = {"SNSP-", code, "-", F};
    if(D == 'H') revision = {"SNSP-", code, "-", F};
    if(D == 'I') revision = {"SNSP-", code, "-", F};
    if(D == 'J') revision = {"SHVC-", code, "-", F};
    if(D == 'K') revision = {"SNSN-", code, "-", F};
    if(D == 'N') revision = {"SNS-",  code, "-", F};
    if(D == 'P') revision = {"SNSP-", code, "-", F};
    if(D == 'S') revision = {"SNSP-", code, "-", F};
    if(D == 'U') revision = {"SNSP-", code, "-", F};
    if(D == 'W') revision = {"SNSP-", code, "-", F};
  }

  if(!revision) {
    revision = {"1.", F};
  }

  return revision ? revision : string{"1.", F};
}

//format: [slot]-[coprocessor]-[mapper]-[ram]-[rtc]
auto SuperFamicom::board() const -> string {
  string board;

  auto mapMode          = data[headerAddress + 0x25];
  auto cartridgeTypeLo  = data[headerAddress + 0x26] & 15;
  auto cartridgeTypeHi  = data[headerAddress + 0x26] >> 4;
  auto cartridgeSubType = data[headerAddress + 0x0f];

  string mode;
  if(mapMode == 0x20 || mapMode == 0x30) mode = "LOROM-";
  if(mapMode == 0x21 || mapMode == 0x31) mode = "HIROM-";
  if(mapMode == 0x22 || mapMode == 0x32) mode = "SDD1-";
  if(mapMode == 0x23 || mapMode == 0x33) mode = "SA1-";
  if(mapMode == 0x25 || mapMode == 0x35) mode = "EXHIROM-";
  if(mapMode == 0x2a || mapMode == 0x3a) mode = "SPC7110-";

  //many games will store an extra title character, overwriting the map mode
  //further, ExLoROM mode is unofficial, and lacks a mapping mode value
  if(!mode) {
    if(headerAddress ==   0x7fb0) mode = "LOROM-";
    if(headerAddress ==   0xffb0) mode = "HIROM-";
    if(headerAddress == 0x407fb0) mode = "EXLOROM-";
    if(headerAddress == 0x40ffb0) mode = "EXHIROM-";
  }

  //the Sufami Turbo has the non-descriptive label "ADD-ON BASE CASSETE"
  //(yes, missing a T), and its serial "A9PJ" is shared with
  //Bishoujo Senshi Sailor Moon SuperS - Fuwafuwa Panic (Japan)
  //so we identify it with this embedded string
  string sufamiSignature = "BANDAI SFC-ADX";
  if(string(data.view(0, sufamiSignature.length())) == sufamiSignature) board.append("ST-", mode);

  //this game's title overwrite the map mode with '!' (0x21), but is a LOROM game
  if(label() == "YUYU NO QUIZ DE GO!GO") mode = "LOROM-";

  //some ExLoROM titles have mode = LOROM-, let's fix that
  if(mode == "LOROM-" && headerAddress == 0x407fb0) mode = "EXLOROM-";

  bool epsonRTC = false;
  bool sharpRTC = false;

  if(serial() == "ZBSJ") {
  //BS-X: Sore wa Namae o Nusumareta Machi no Monogatari (JPN)
    board.append("BS-MCC-");
  } else if(serial() == "042J") {
  //Super Game Boy 2
    board.append("GB-", mode);
  } else if(serial().match("Z??J")) {
    board.append("BS-", mode);
  } else if(cartridgeTypeLo >= 0x3) {
    if(cartridgeTypeHi == 0x0) board.append("NEC-", mode);
    if(cartridgeTypeHi == 0x1) board.append("GSU-");
    if(cartridgeTypeHi == 0x2) board.append("OBC1-", mode);
    if(cartridgeTypeHi == 0x3) board.append("SA1-");
    if(cartridgeTypeHi == 0x4) board.append("SDD1-");
    if(cartridgeTypeHi == 0x5) board.append(mode), sharpRTC = true;
    if(cartridgeTypeHi == 0xe && cartridgeTypeLo == 0x3) board.append("GB-", mode);
    if(cartridgeTypeHi == 0xf && cartridgeTypeLo == 0x5 && cartridgeSubType == 0x00) board.append("SPC7110-");
    if(cartridgeTypeHi == 0xf && cartridgeTypeLo == 0x9 && cartridgeSubType == 0x00) board.append("SPC7110-"), epsonRTC = true;
    if(cartridgeTypeHi == 0xf                           && cartridgeSubType == 0x01) board.append("EXNEC-", mode);
    if(cartridgeTypeHi == 0xf                           && cartridgeSubType == 0x02) board.append("ARM-", mode);
    if(cartridgeTypeHi == 0xf                           && cartridgeSubType == 0x10) board.append("HITACHI-", mode);
  }
  if(!board) board.append(mode);

  if(ramSize() || expansionRamSize()) board.append("RAM-");
  if(epsonRTC) board.append("EPSONRTC-");
  if(sharpRTC) board.append("SHARPRTC-");

  board.trimRight("-", 1L);

  if(board.beginsWith(    "LOROM-RAM") && romSize() <= 0x200000) board.append("#A");
  if(board.beginsWith("NEC-LOROM-RAM") && romSize() <= 0x100000) board.append("#A");

  //Tengai Makyou Zero (fan translation)
  if(board.beginsWith("SPC7110-") && data.size() == 0x700000) board.prepend("EX");

  return board;
}

auto SuperFamicom::label() const -> string {
  string label;

  for(u32 n = 0; n < 0x15; n++) {
    auto x = data[headerAddress + 0x10 + n];
    auto y = n == 0x14 ? 0 : data[headerAddress + 0x11 + n];

    //null terminator (padding)
    if(x == 0x00 || x == 0xff);

    //ASCII
    else if(x >= 0x20 && x <= 0x7e) label.append((char)x);

    //Shift-JIS (half-width katakana)
    else if(x == 0xa1) label.append("。");
    else if(x == 0xa2) label.append("「");
    else if(x == 0xa3) label.append("」");
    else if(x == 0xa4) label.append("、");
    else if(x == 0xa5) label.append("・");
    else if(x == 0xa6) label.append("ヲ");
    else if(x == 0xa7) label.append("ァ");
    else if(x == 0xa8) label.append("ィ");
    else if(x == 0xa9) label.append("ゥ");
    else if(x == 0xaa) label.append("ェ");
    else if(x == 0xab) label.append("ォ");
    else if(x == 0xac) label.append("ャ");
    else if(x == 0xad) label.append("ュ");
    else if(x == 0xae) label.append("ョ");
    else if(x == 0xaf) label.append("ッ");
    else if(x == 0xb0) label.append("ー");

    else if(x == 0xb1) label.append(                 "ア");
    else if(x == 0xb2) label.append(                 "イ");
    else if(x == 0xb3) label.append(y == 0xde ? "ヴ" : "ウ");
    else if(x == 0xb4) label.append(                 "エ");
    else if(x == 0xb5) label.append(                 "オ");

    else if(x == 0xb6) label.append(y == 0xde ? "ガ" : "カ");
    else if(x == 0xb7) label.append(y == 0xde ? "ギ" : "キ");
    else if(x == 0xb8) label.append(y == 0xde ? "グ" : "ク");
    else if(x == 0xb9) label.append(y == 0xde ? "ゲ" : "ケ");
    else if(x == 0xba) label.append(y == 0xde ? "ゴ" : "コ");

    else if(x == 0xbb) label.append(y == 0xde ? "ザ" : "サ");
    else if(x == 0xbc) label.append(y == 0xde ? "ジ" : "シ");
    else if(x == 0xbd) label.append(y == 0xde ? "ズ" : "ス");
    else if(x == 0xbe) label.append(y == 0xde ? "ゼ" : "セ");
    else if(x == 0xbf) label.append(y == 0xde ? "ゾ" : "ソ");

    else if(x == 0xc0) label.append(y == 0xde ? "ダ" : "タ");
    else if(x == 0xc1) label.append(y == 0xde ? "ヂ" : "チ");
    else if(x == 0xc2) label.append(y == 0xde ? "ヅ" : "ツ");
    else if(x == 0xc3) label.append(y == 0xde ? "デ" : "テ");
    else if(x == 0xc4) label.append(y == 0xde ? "ド" : "ト");

    else if(x == 0xc5) label.append("ナ");
    else if(x == 0xc6) label.append("ニ");
    else if(x == 0xc7) label.append("ヌ");
    else if(x == 0xc8) label.append("ネ");
    else if(x == 0xc9) label.append("ノ");

    else if(x == 0xca) label.append(y == 0xdf ? "パ" : y == 0xde ? "バ" : "ハ");
    else if(x == 0xcb) label.append(y == 0xdf ? "ピ" : y == 0xde ? "ビ" : "ヒ");
    else if(x == 0xcc) label.append(y == 0xdf ? "プ" : y == 0xde ? "ブ" : "フ");
    else if(x == 0xcd) label.append(y == 0xdf ? "ペ" : y == 0xde ? "ベ" : "ヘ");
    else if(x == 0xce) label.append(y == 0xdf ? "ポ" : y == 0xde ? "ボ" : "ホ");

    else if(x == 0xcf) label.append("マ");
    else if(x == 0xd0) label.append("ミ");
    else if(x == 0xd1) label.append("ム");
    else if(x == 0xd2) label.append("メ");
    else if(x == 0xd3) label.append("モ");

    else if(x == 0xd4) label.append("ヤ");
    else if(x == 0xd5) label.append("ユ");
    else if(x == 0xd6) label.append("ヨ");

    else if(x == 0xd7) label.append("ラ");
    else if(x == 0xd8) label.append("リ");
    else if(x == 0xd9) label.append("ル");
    else if(x == 0xda) label.append("レ");
    else if(x == 0xdb) label.append("ロ");

    else if(x == 0xdc) label.append("ワ");
    else if(x == 0xdd) label.append("ン");

    else if(x == 0xde) label.append("\xef\xbe\x9e");  //dakuten
    else if(x == 0xdf) label.append("\xef\xbe\x9f");  //handakuten

    //unknown
    else label.append("?");

    //(han)dakuten skip
    if(y == 0xde && x == 0xb3) n++;
    if(y == 0xde && x >= 0xb6 && x <= 0xc4) n++;
    if(y == 0xde && x >= 0xca && x <= 0xce) n++;
    if(y == 0xdf && x >= 0xca && y <= 0xce) n++;
  }

  return label.strip();
}

auto SuperFamicom::serial() const -> string {
  char A = data[headerAddress + 0x02];  //game type
  char B = data[headerAddress + 0x03];  //game code
  char C = data[headerAddress + 0x04];  //game code
  char D = data[headerAddress + 0x05];  //region code (new; sometimes ambiguous)

  auto valid = [](char n) { return (n >= '0' && n <= '9') || (n >= 'A' && n <= 'Z'); };
  if(data[headerAddress + 0x2a] == 0x33 && valid(A) && valid(B) & valid(C) & valid(D)) {
    return {A, B, C, D};
  }

  return "";
}

auto SuperFamicom::romSize() const -> u32 {
  return size() - firmwareRomSize();
}

auto SuperFamicom::programRomSize() const -> u32 {
  if(board().beginsWith("SPC7110-")) return 0x100000;
  if(board().beginsWith("EXSPC7110-")) return 0x100000;
  return romSize();
}

auto SuperFamicom::dataRomSize() const -> u32 {
  if(board().beginsWith("SPC7110-")) return romSize() - 0x100000;
  if(board().beginsWith("EXSPC7110-")) return 0x500000;
  return 0;
}

auto SuperFamicom::expansionRomSize() const -> u32 {
  if(board().beginsWith("EXSPC7110-")) return 0x100000;
  return 0;
}

//detect if any firmware is appended to the ROM image, and return its size if so
auto SuperFamicom::firmwareRomSize() const -> u32 {
  auto cartridgeTypeLo  = data[headerAddress + 0x26] & 15;
  auto cartridgeTypeHi  = data[headerAddress + 0x26] >> 4;
  auto cartridgeSubType = data[headerAddress + 0x0f];

  if(serial() == "042J" || (cartridgeTypeLo == 0x3 && cartridgeTypeHi == 0xe)) {
    //Game Boy
    if((size() & 0x7fff) == 0x100) return 0x100;
  }

  if(cartridgeTypeLo >= 0x3 && cartridgeTypeHi == 0xf && cartridgeSubType == 0x10) {
    //Hitachi HG51BS169
    if((size() & 0x7fff) == 0xc00) return 0xc00;
  }

  if(cartridgeTypeLo >= 0x3 && cartridgeTypeHi == 0x0) {
    //NEC uPD7725
    if((size() & 0x7fff) == 0x2000) return 0x2000;
  }

  if(cartridgeTypeLo >= 0x3 && cartridgeTypeHi == 0xf && cartridgeSubType == 0x01) {
    //NEC uPD96050
    if((size() & 0xffff) == 0xd000) return 0xd000;
  }

  if(cartridgeTypeLo >= 0x3 && cartridgeTypeHi == 0xf && cartridgeSubType == 0x02) {
    //ARM6
    if((size() & 0x3ffff) == 0x28000) return 0x28000;
  }

  return 0;
}

auto SuperFamicom::ramSize() const -> u32 {
  auto ramSize = data[headerAddress + 0x28] & 7;
  if(ramSize) return 1024 << ramSize;
  return 0;
}

auto SuperFamicom::expansionRamSize() const -> u32 {
  if(data[headerAddress + 0x2a] == 0x33) {
    auto ramSize = data[headerAddress + 0x0d] & 7;
    if(ramSize) return 1024 << ramSize;
  }
  if((data[headerAddress + 0x26] >> 4) == 1) {
    //GSU: Starfox / Starwing lacks an extended header; but still has expansion RAM
    return 0x8000;
  }
  return 0;
}

auto SuperFamicom::nonVolatile() const -> bool {
  auto cartridgeTypeLo = data[headerAddress + 0x26] & 15;
  return cartridgeTypeLo == 0x2 || cartridgeTypeLo == 0x5 || cartridgeTypeLo == 0x6;
}

auto SuperFamicom::scoreHeader(u32 address) -> u32 {
  s32 score = 0;
  if(size() < address + 0x50) return score;

  u8  mapMode     = data[address + 0x25] & ~0x10;  //ignore FastROM bit
  u16 complement  = data[address + 0x2c] << 0 | data[address + 0x2d] << 8;
  u16 checksum    = data[address + 0x2e] << 0 | data[address + 0x2f] << 8;
  u16 resetVector = data[address + 0x4c] << 0 | data[address + 0x4d] << 8;
  if(resetVector < 0x8000) return score;  //$00:0000-7fff is never ROM data

  u8 opcode = data[(address & ~0x7fff) | (resetVector & 0x7fff)];  //first instruction executed

  //most likely opcodes
  if(opcode == 0x78  //sei
  || opcode == 0x18  //clc (clc; xce)
  || opcode == 0x38  //sec (sec; xce)
  || opcode == 0x9c  //stz $nnnn (stz $4200)
  || opcode == 0x4c  //jmp $nnnn
  || opcode == 0x5c  //jml $nnnnnn
  ) score += 8;

  //plausible opcodes
  if(opcode == 0xc2  //rep #$nn
  || opcode == 0xe2  //sep #$nn
  || opcode == 0xad  //lda $nnnn
  || opcode == 0xae  //ldx $nnnn
  || opcode == 0xac  //ldy $nnnn
  || opcode == 0xaf  //lda $nnnnnn
  || opcode == 0xa9  //lda #$nn
  || opcode == 0xa2  //ldx #$nn
  || opcode == 0xa0  //ldy #$nn
  || opcode == 0x20  //jsr $nnnn
  || opcode == 0x22  //jsl $nnnnnn
  ) score += 4;

  //implausible opcodes
  if(opcode == 0x40  //rti
  || opcode == 0x60  //rts
  || opcode == 0x6b  //rtl
  || opcode == 0xcd  //cmp $nnnn
  || opcode == 0xec  //cpx $nnnn
  || opcode == 0xcc  //cpy $nnnn
  ) score -= 4;

  //least likely opcodes
  if(opcode == 0x00  //brk #$nn
  || opcode == 0x02  //cop #$nn
  || opcode == 0xdb  //stp
  || opcode == 0x42  //wdm
  || opcode == 0xff  //sbc $nnnnnn,x
  ) score -= 8;

  if(checksum + complement == 0xffff) score += 4;

  if(address == 0x7fb0 && mapMode == 0x20) score += 2;
  if(address == 0xffb0 && mapMode == 0x21) score += 2;

  return max(0, score);
}

auto SuperFamicom::firmwareARM() const -> string {
  return "ST018";
}

auto SuperFamicom::firmwareEXNEC() const -> string {
  if(label() == "EXHAUST HEAT2") return "ST010";
  if(label() == "F1 ROC II") return "ST010";
  if(label() == "2DAN MORITA SHOUGI") return "ST011";
  return "ST010";
}

auto SuperFamicom::firmwareGB() const -> string {
  if(label() == "Super GAMEBOY") return "SGB1";
  if(label() == "Super GAMEBOY2") return "SGB2";
  return "SGB1";
}

auto SuperFamicom::firmwareHITACHI() const -> string {
  return "Cx4";
}

auto SuperFamicom::firmwareNEC() const -> string {
  if(label() == "PILOTWINGS") return "DSP1";
  if(label() == "DUNGEON MASTER") return "DSP2";
  if(label() == "SDガンダムGX") return "DSP3";
  if(label() == "PLANETS CHAMP TG3000") return "DSP4";
  if(label() == "TOP GEAR 3000") return "DSP4";
  return "DSP1B";
}
