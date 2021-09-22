struct MegaDrive : Cartridge {
  auto name() -> string override { return "Mega Drive"; }
  auto extensions() -> vector<string> override { return {"md", "smd", "gen", "bin"}; }
  auto load(string location) -> bool override;
  auto save(string location) -> bool override;
  auto analyze(vector<u8>& rom) -> string;
  auto analyzeStorage(vector<u8>& rom, string hash) -> void;
  auto analyzePeripherals(vector<u8>& rom, string hash) -> void;
  auto analyzeCopyProtection(vector<u8>& rom, string hash) -> void;

  struct RAM {
    explicit operator bool() const { return mode && size != 0; }

    string mode;
    u32 size = 0;
  } ram;

  struct EEPROM {
    explicit operator bool() const { return mode && size != 0; }

    string mode;
    u32 size = 0;
    u8 rsda = 0;
    u8 wsda = 0;
    u8 wscl = 0;
  } eeprom;

  struct Peripherals {
    bool jcart = 0;
  } peripherals;
};

auto MegaDrive::load(string location) -> bool {
  vector<u8> rom;
  if(directory::exists(location)) {
    append(rom, {location, "program.rom"});
  } else if(file::exists(location)) {
    rom = Cartridge::read(location);
  }
  if(!rom) return false;

  this->location = location;
  this->manifest = analyze(rom);
  auto document = BML::unserialize(manifest);
  if(!document) return false;

  pak = new vfs::directory;
  pak->setAttribute("title",    document["game/title"].string());
  pak->setAttribute("region",   document["game/region"].string());
  pak->setAttribute("bootable", true);
  pak->setAttribute("megacd",   (bool)document["game/device"].string().split(", ").find("Mega CD"));
  pak->append("manifest.bml", manifest);

  //add SVP ROM to image if it is missing
  if(document["game/board/memory(type=ROM,content=SVP)"]) {
    if(memory::compare(rom.data() + rom.size() - 0x800, Resource::MegaDrive::SVP, 0x800)) {
      rom.resize(rom.size() + 0x800);
      memory::copy(rom.data() + rom.size() - 0x800, Resource::MegaDrive::SVP, 0x800);
    }
  }

  array_view<u8> view{rom};
  for(auto node : document.find("game/board/memory(type=ROM)")) {
    string name = {node["content"].string().downcase(), ".rom"};
    u32 size = node["size"].natural();
    if(view.size() < size) break;  //missing firmware
    pak->append(name, {view.data(), size});
    view += size;
  }

  if(auto node = document["game/board/memory(type=RAM,content=Save)"]) {
    Medium::load(node, ".ram");
    if(auto fp = pak->read("save.ram")) {
      fp->setAttribute("mode", node["mode"].string());
    }
  }

  if(auto node = document["game/board/memory(type=EEPROM,content=Save)"]) {
    Medium::load(node, ".eeprom");
    if(auto fp = pak->read("save.eeprom")) {
      fp->setAttribute("mode", node["mode"].string());
      fp->setAttribute("rsda", node["rsda"].natural());
      fp->setAttribute("wsda", node["wsda"].natural());
      fp->setAttribute("wscl", node["wscl"].natural());
    }
  }

  if(auto node = document["game/board(peripheral=J-Cart)"]) {
    pak->setAttribute("jcart", true);
  }

  return true;
}

auto MegaDrive::save(string location) -> bool {
  auto document = BML::unserialize(manifest);

  if(auto node = document["game/board/memory(type=RAM,content=Save)"]) {
    Medium::save(node, ".ram");
  }

  if(auto node = document["game/board/memory(type=EEPROM,content=Save)"]) {
    Medium::save(node, ".eeprom");
  }

  return true;
}

auto MegaDrive::analyze(vector<u8>& rom) -> string {
  if(rom.size() < 0x800) return {};

  ram = {};
  eeprom = {};
  peripherals = {};

  auto hash = Hash::SHA256(rom).digest();
  analyzeStorage(rom, hash);
  analyzePeripherals(rom, hash);
  analyzeCopyProtection(rom, hash);

  vector<string> devices;
  string device = slice((const char*)&rom[0x190], 0, 16).trimRight(" ");
  for(auto& id : device) {
    if(id == '0');  //Master System controller
    if(id == '4');  //multitap
    if(id == '6');  //6-button controller
    if(id == 'A');  //analog joystick
    if(id == 'B');  //trackball
    if(id == 'C') devices.append("Mega CD");
    if(id == 'D');  //download?
    if(id == 'F');  //floppy drive
    if(id == 'G');  //light gun
    if(id == 'J');  //3-button controller
    if(id == 'K');  //keyboard
    if(id == 'L');  //Activator
    if(id == 'M');  //mouse
    if(id == 'P');  //printer
    if(id == 'R');  //RS-232 modem
    if(id == 'T');  //tablet
    if(id == 'V');  //paddle
  }

  vector<string> regions;
  string region = slice((const char*)&rom[0x01f0], 0, 16).trimRight(" ");
  if(!regions) {
    if(region == "JAPAN" ) regions.append("NTSC-J");
    if(region == "EUROPE") regions.append("PAL");
  }
  if(!regions) {
    if(region.find("J")) regions.append("NTSC-J");
    if(region.find("U")) regions.append("NTSC-U");
    if(region.find("E")) regions.append("PAL");
  }
  if(!regions && region.size() == 1) {
    maybe<u8> bits;
    u8 field = region[0];
    if(field >= '0' && field <= '9') bits = field - '0';
    if(field >= 'A' && field <= 'F') bits = field - 'A' + 10;
    if(bits && *bits & 1) regions.append("NTSC-J");  //domestic 60hz
    if(bits && *bits & 2);                           //domestic 50hz
    if(bits && *bits & 4) regions.append("NTSC-U");  //overseas 60hz
    if(bits && *bits & 8) regions.append("PAL");     //overseas 50hz
  }
  if(!regions) {
    regions.append("NTSC-J", "NTSC-U", "PAL");
  }

  string domesticName;
  domesticName.resize(48);
  memory::copy(domesticName.get(), &rom[0x0120], domesticName.size());
  for(auto& c : domesticName) if(c < 0x20 || c > 0x7e) c = ' ';
  while(domesticName.find("  ")) domesticName.replace("  ", " ");
  domesticName.strip();

  string internationalName;
  internationalName.resize(48);
  memory::copy(internationalName.get(), &rom[0x0150], internationalName.size());
  for(auto& c : internationalName) if(c < 0x20 || c > 0x7e) c = ' ';
  while(internationalName.find("  ")) internationalName.replace("  ", " ");
  internationalName.strip();

  string serialNumber;
  serialNumber.resize(14);
  memory::copy(serialNumber.get(), &rom[0x180], serialNumber.size());
  for(auto& c : serialNumber) if(c < 0x20 || c > 0x7e) c = ' ';
  while(serialNumber.find("  ")) serialNumber.replace("  ", " ");
  serialNumber.strip();

  string s;
  s += "game\n";
  s +={"  sha256: ", hash, "\n"};
  s +={"  name:   ", Medium::name(location), "\n"};
  s +={"  title:  ", Medium::name(location), "\n"};
  s +={"  label:  ", domesticName, "\n"};
  s +={"  label:  ", internationalName, "\n"};
  s +={"  serial: ", serialNumber, "\n"};
  s +={"  region: ", regions.merge(", "), "\n"};
  if(devices)
  s +={"  device: ", devices.merge(", "), "\n"};
  s += "  board\n";

  if(domesticName == "Game Genie") {
    s += "    memory\n";
    s += "      type: ROM\n";
    s +={"      size: 0x", hex(rom.size()), "\n"};
    s += "      content: Program\n";
    s += "    slot\n";
    s += "      type: Mega Drive\n";
  } else if(internationalName == "Virtua Racing") {
    s += "    memory\n";
    s += "      type: ROM\n";
    s +={"      size: 0x", hex(rom.size()), "\n"};
    s += "      content: Program\n";
    s += "    memory\n";
    s += "      type: ROM\n";
    s += "      size: 0x800\n";
    s += "      content: SVP\n";
  } else {
    s += "    memory\n";
    s += "      type: ROM\n";
    s +={"      size: 0x", hex(rom.size()), "\n"};
    s += "      content: Program\n";
  }

  if(eeprom) {
    s += "    memory\n";
    s += "      type: EEPROM\n";
    s +={"      size: 0x", hex(eeprom.size), "\n"};
    s += "      content: Save\n";
    s +={"      mode: ", eeprom.mode, "\n"};
    s +={"      rsda: ", eeprom.rsda, "\n"};
    s +={"      wsda: ", eeprom.wsda, "\n"};
    s +={"      wscl: ", eeprom.wscl, "\n"};
  } else if(ram) {
    s += "    memory\n";
    s += "      type: RAM\n";
    s +={"      size: 0x", hex(ram.size), "\n"};
    s += "      content: Save\n";
    s +={"      mode: ", ram.mode, "\n"};
  }

  if(peripherals.jcart) {
    s += "    peripheral: J-Cart";
  }

  return s;
}

auto MegaDrive::analyzeStorage(vector<u8>& rom, string hash) -> void {
  //SRAM
  //====

  if(rom[0x01b0] == 'R' && rom[0x01b1] == 'A') {
    u32 ramFrom = 0;
    ramFrom |= rom[0x01b4] << 24;
    ramFrom |= rom[0x01b5] << 16;
    ramFrom |= rom[0x01b6] <<  8;
    ramFrom |= rom[0x01b7] <<  0;

    u32 ramTo = 0;
    ramTo |= rom[0x01b8] << 24;
    ramTo |= rom[0x01b9] << 16;
    ramTo |= rom[0x01ba] <<  8;
    ramTo |= rom[0x01bb] <<  0;

    if(!(ramFrom & 1) && !(ramTo & 1)) ram.mode = "upper";
    if( (ramFrom & 1) &&  (ramTo & 1)) ram.mode = "lower";
    if(!(ramFrom & 1) &&  (ramTo & 1)) ram.mode = "word";

    if(ram.mode == "upper") ram.size = (ramTo - ramFrom + 2) >> 1;
    if(ram.mode == "lower") ram.size = (ramTo - ramFrom + 2) >> 1;
    if(ram.mode == "word" ) ram.size = (ramTo - ramFrom + 1);
  }

  //Buck Rogers: Countdown to Doomsday (USA, Europe)
  if(hash == "997cbd682bc7c0636302f07219d9152244c8ae06028fd7d91463d35756630ce5") {
    ram.mode = "lower";
    ram.size = 8192;
  }

  //Hardball III (USA)
  if(hash == "bba8186e38d0d0938a439bda4fc2325c1b2c9760a643717fa4f372ebf9058fc2") {
    ram.mode = "lower";
    ram.size = 32768;
  }

  //Madden NFL '98 (USA)
  if(hash == "27bdbb05279c52624f77e361e8a2bfb41623f4a63260104d3a89f44c8276adb2") {
    ram.mode = "lower";
    ram.size = 16384;
  }

  //Might and Magic: Gates to Another World (USA, Europe)
  if(hash == "d86b6d7381ef67ecb5391eddb6857bf9d15b1e402da6bfc42cb003186599cbff") {
    ram.mode = "lower";
    ram.size = 32768;
  }

  //Might and Magic III: Isles of Terra (USA)
  if(hash == "ac08551ecd4c037211fca98359efcfe7c0b048880e82a474d5c5fcd157e33592") {
    ram.mode = "lower";
    ram.size = 32768;
  }

  //M28C16
  //======

  //Barkley Shut Up and Jam 2 (USA)
  if(hash == "637b400fe79830368feb52c3d21fd5badeddfe6cea08b7e07fa8f6d34822e5b8") {
    ram.mode = "lower";
    ram.size = 2_KiB;
  }

  //Barkley Shut Up and Jam 2 (USA) (Beta)
  if(hash == "3b94a16d93f8d6656a0d4f5e3370dffe510e3d985e20c26b71873beb5403dfd0") {
    ram.mode = "lower";
    ram.size = 2_KiB;
  }

  //Unnecessary Roughness '95
  if(hash == "7d99fe9dd35dc5c9a124cefe60204f3c0cc74e81269c57e635e1029dd68274f7") {
    ram.mode = "lower";
    ram.size = 2_KiB;
  }

  //X24C01
  //======

  //Bill Walsh College Football (USA, Europe)
  if(hash == "b90e9d7ecd74980c422ed72575a36d4d378302136faafbc365474e5d037b30fa") {
    eeprom.mode = "X24C01";
    eeprom.size = 128;
    eeprom.rsda = 7;
    eeprom.wsda = 7;
    eeprom.wscl = 6;
  }

  //Evander Holyfield's 'Real Deal' Boxing (World)
  if(hash == "b13f1e31bf6ba739e4cffd98374263b3421b3a61c4873e30dbc866198736e880") {
    eeprom.mode = "X24C01";
    eeprom.size = 128;
    eeprom.rsda = 0;
    eeprom.wsda = 0;
    eeprom.wscl = 1;
  }

  //Game Toshokan (Japan) (1.1)
  if(hash == "81a30f887132792896796d0aebce628d7c092f3b2642b83bf2fc78f7c940ce34") {
    eeprom.mode = "X24C01";
    eeprom.size = 128;
    eeprom.rsda = 0;
    eeprom.wsda = 0;
    eeprom.wscl = 1;
  }

  //Greatest Heavyweights (Japan)
  if(hash == "c2daef4bb84f6d4e52caf9a28d9f2561642d3b7e749dc62f12a2587ee77d1222") {
    eeprom.mode = "X24C01";
    eeprom.size = 128;
    eeprom.rsda = 0;
    eeprom.wsda = 0;
    eeprom.wscl = 1;
  }

  //Greatest Heavyweights (USA)
  if(hash == "2007029b0fb227feaa7ef2de7e189270e52c81bc291241a4b5130c78993bc7b9") {
    eeprom.mode = "X24C01";
    eeprom.size = 128;
    eeprom.rsda = 0;
    eeprom.wsda = 0;
    eeprom.wscl = 1;
  }

  //Greatest Heavyweights (Europe)
  if(hash == "18920dea1147e1cc0fbafc0ee15524627c47f4211eebf14d4eff36d0b4d7e3f8") {
    eeprom.mode = "X24C01";
    eeprom.size = 128;
    eeprom.rsda = 0;
    eeprom.wsda = 0;
    eeprom.wscl = 1;
  }

  //Honoo no Toukyuuji: Dodge Danpei (Japan)
  if(hash == "0d476b9f1c7245d408a2c1a6d22bc0f37e7fe3be9850b7d9c9fac5b18f616511") {
    eeprom.mode = "X24C01";
    eeprom.size = 128;
    eeprom.rsda = 0;
    eeprom.wsda = 0;
    eeprom.wscl = 1;
  }

  //John Madden Football '93 (USA, Europe)
  if(hash == "8958d7d382aa99d91122564397a5aa438c26f4bab2c20626756f7802b2467624") {
    eeprom.mode = "X24C01";
    eeprom.size = 128;
    eeprom.rsda = 7;
    eeprom.wsda = 7;
    eeprom.wscl = 6;
  }

  //John Madden Football '93: Championship Edition (USA)
  if(hash == "cfb45fad6dada893ae2ac93eb5bbd36d2de6c5973430f4018cdcb1a53fdfdb12") {
    eeprom.mode = "X24C01";
    eeprom.size = 128;
    eeprom.rsda = 7;
    eeprom.wsda = 7;
    eeprom.wscl = 6;
  }

  //Mega Man: The Wily Wars (Europe)
  if(hash == "98f36274e4d73feefaba0e812024a6cecda2d864a2cbed7363b9d357f2e6a582") {
    eeprom.mode = "X24C01";
    eeprom.size = 128;
    eeprom.rsda = 0;
    eeprom.wsda = 0;
    eeprom.wscl = 1;
  }

  //NHLPA Hockey '93 (USA, Europe) (1.0)
  if(hash == "524e7e61c6096e920ca263a4ed208c002a4ef6cfc17bfda60bf9ecb63c975951") {
    eeprom.mode = "X24C01";
    eeprom.size = 128;
    eeprom.rsda = 7;
    eeprom.wsda = 7;
    eeprom.wscl = 6;
  }

  //NHLPA Hockey '93 (USA, Europe) (1.1)
  if(hash == "69a05a0f73bc6b3f5b5c7aff53bb0a1db8fab4af8eee0c84d92e503214a156a1") {
    eeprom.mode = "X24C01";
    eeprom.size = 128;
    eeprom.rsda = 7;
    eeprom.wsda = 7;
    eeprom.wscl = 6;
  }

  //Ninja Burai Densetsu (Japan)
  if(hash == "212c70be20c8a29f11c5392a06207e43c2bff07e716a4d3cbf2d046f684a3cef") {
    eeprom.mode = "X24C01";
    eeprom.size = 128;
    eeprom.rsda = 0;
    eeprom.wsda = 0;
    eeprom.wscl = 1;
  }

  //Rings of Power (USA, Europe)
  if(hash == "36303fc447c433ebc69c3d4df86c783c86b383e0acead1c19595f13269e248f5") {
    eeprom.mode = "X24C01";
    eeprom.size = 128;
    eeprom.rsda = 7;
    eeprom.wsda = 7;
    eeprom.wscl = 6;
  }

  //Rockman: Mega World (Japan) (1.1)
  if(hash == "390dcd13bbde5e5aab352c456c3ab7d40df60edaae65687cda4598cb1387a3d6") {
    eeprom.mode = "X24C01";
    eeprom.size = 128;
    eeprom.rsda = 0;
    eeprom.wsda = 0;
    eeprom.wscl = 1;
  }

  //Sports Talk Baseball (USA)
  if(hash == "553eb97741c52e2c3ce668960c1f70231514f7fa3532f7cadcf370bb28ff1efe") {
    eeprom.mode = "X24C01";
    eeprom.size = 128;
    eeprom.rsda = 0;
    eeprom.wsda = 0;
    eeprom.wscl = 1;
  }

  //Wonder Boy in Monster World (USA, Europe)
  if(hash == "6b2ac36f624f914ad26e32baa87d1253aea9dcfc13d2a5842ecdd2bd4a7a43b9") {
    eeprom.mode = "X24C01";
    eeprom.size = 128;
    eeprom.rsda = 0;
    eeprom.wsda = 0;
    eeprom.wscl = 1;
  }

  //X24C02
  //======

  //NBA Jam (Japan)
  if(hash == "5aa7e5ccb1931c688f4f0cadea5d4b41440c6c4bbaad83b6bab547ceede31c1d") {
    eeprom.mode = "X24C02";
    eeprom.size = 256;
    eeprom.rsda = 1;
    eeprom.wsda = 0;
    eeprom.wscl = 1;
  }

  //NBA Jam (USA, Europe) (1.0)
  if(hash == "9ee77c144505a10940976bb48e32600c83d5910c737612868ff6088faf3eb7f4") {
    eeprom.mode = "X24C02";
    eeprom.size = 256;
    eeprom.rsda = 1;
    eeprom.wsda = 0;
    eeprom.wscl = 1;
  }

  //NBA Jam (USA, Europe) (1.1)
  if(hash == "d90d4ca8d00b4dade6d21650da2264fa64016daa779215ac06adfc04da31d610") {
    eeprom.mode = "X24C02";
    eeprom.size = 256;
    eeprom.rsda = 1;
    eeprom.wsda = 0;
    eeprom.wscl = 1;
  }

  //M24C02
  //======

  //NFL Quarterback Club (World)
  if(hash == "a24174cdbe188efabda887f4c7ad492d0cb36b74265a90b2af8c2a5b480f4e7f") {
    eeprom.mode = "M24C02";
    eeprom.size = 256;
    eeprom.rsda = 0;
    eeprom.wsda = 0;
    eeprom.wscl = 8;
  }

  //M24C04
  //======

  //Blockbuster World Video Game Championship II (USA)
  if(hash == "8bdd2fa68e70f727f6c86f8266ec703457e09c2940cbd8f82c3760e2177b88fe") {
    eeprom.mode = "M24C04";
    eeprom.size = 512;
    eeprom.rsda = 0;
    eeprom.wsda = 0;
    eeprom.wscl = 8;
  }

  //NBA Jam: Tournament Edition (World)
  if(hash == "5b735b443102f771bc7db49dcf34de640c10d212f5cb4af1251d956751f21072") {
    eeprom.mode = "M24C04";
    eeprom.size = 512;
    eeprom.rsda = 0;
    eeprom.wsda = 0;
    eeprom.wscl = 8;
  }

  //M24C08
  //======

  //Brian Lara Cricket (Europe) (1.0)
  if(hash == "d52223c45d0c29a00003a9782227b775b66640dbfc92bdb9346565c4f97b8cad") {
    eeprom.mode = "M24C08";
    eeprom.size = 1024;
    eeprom.rsda = 7;
    eeprom.wsda = 0;
    eeprom.wscl = 1;
  }

  //Brian Lara Cricket (Europe) (1.1)
  if(hash == "1b28d7f1a8eb4c43afa06de8ea98290368b6ec00ec0c1c16de27ce33d892a051") {
    eeprom.mode = "M24C08";
    eeprom.size = 1024;
    eeprom.rsda = 7;
    eeprom.wsda = 0;
    eeprom.wscl = 1;
  }

  //Micro Machines: Military (Europe)
  if(hash == "43e68f99620bb3dd0ff3b28d232ae43c5b6489716c4cde09725ab4b71704937e") {
    eeprom.mode = "M24C08";
    eeprom.size = 1024;
    eeprom.rsda = 7;
    eeprom.wsda = 0;
    eeprom.wscl = 1;
  }

  //M24C16
  //======

  //Micro Machines: Turbo Tournament '96 (Europe) (1.0)
  if(hash == "ec3b97adf166b2e4e9ef402ac79ce9d458a932996b7fd259cd55db81adb496a6") {
    eeprom.mode = "M24C16";
    eeprom.size = 2048;
    eeprom.rsda = 7;
    eeprom.wsda = 0;
    eeprom.wscl = 1;
  }

  //Micro Machines: Turbo Tournament '96 (Europe) (1.1)
  if(hash == "77ff023aa9f88aeb1605d3060b1331b816752b12b64ff2fe95d4a9f3c5dff946") {
    eeprom.mode = "M24C16";
    eeprom.size = 2048;
    eeprom.rsda = 7;
    eeprom.wsda = 0;
    eeprom.wscl = 1;
  }

  //Micro Machines 2: Turbo Tournament (Europe) (1.0)
  if(hash == "57fa3662f589929d180648090104f8c802894602a6f660755121cb44918f382c") {
    eeprom.mode = "M24C16";
    eeprom.size = 2048;
    eeprom.rsda = 7;
    eeprom.wsda = 0;
    eeprom.wscl = 1;
  }

  //Micro Machines 2: Turbo Tournament (Europe) (1.1)
  if(hash == "9209241472f0e78f23405bb265c7c108c25413d9a01ec1455f4e2d17d80c188c") {
    eeprom.mode = "M24C16";
    eeprom.size = 2048;
    eeprom.rsda = 7;
    eeprom.wsda = 0;
    eeprom.wscl = 1;
  }

  //NFL Quarterback Club '96 (USA, Europe)
  if(hash == "162edaaaf77a3ccdb693c46cf63fdd273eab31ac6b64bc2f450b0c09db9ceef7") {
    eeprom.mode = "M24C16";
    eeprom.size = 2048;
    eeprom.rsda = 0;
    eeprom.wsda = 0;
    eeprom.wscl = 8;
  }

  //M24C65
  //======

  //Brian Lara Cricket '96 (Europe) (1.0)
  if(hash == "2de062b59934a4dc7a4bbc0a012b86a7674597be7ef04f8cb78d088cee8a9eee") {
    eeprom.mode = "M24C65";
    eeprom.size = 8192;
    eeprom.rsda = 7;
    eeprom.wsda = 0;
    eeprom.wscl = 1;
  }

  //Brian Lara Cricket '96 (Europe) (1.1)
  if(hash == "1d36e581171c29cae09457954589916d9e0cfeee193a049f358be428fc7c6421") {
    eeprom.mode = "M24C65";
    eeprom.size = 8192;
    eeprom.rsda = 7;
    eeprom.wsda = 0;
    eeprom.wscl = 1;
  }

  //College Slam (USA)
  if(hash == "52cc1c3e7abed5bb3887fe5a5af96a763a8072faad19a1042a61a2567026d705") {
    eeprom.mode = "M24C65";
    eeprom.size = 8192;
    eeprom.rsda = 0;
    eeprom.wsda = 0;
    eeprom.wscl = 8;
  }

  //Frank Thomas' Big Hurt Baseball (USA, Europe)
  if(hash == "8aa5b0aa8f550a42aa0dfe5c200dcf4bc344e9b114a0c056acced3374441ece0") {
    eeprom.mode = "M24C65";
    eeprom.size = 8192;
    eeprom.rsda = 0;
    eeprom.wsda = 0;
    eeprom.wscl = 8;
  }
}

auto MegaDrive::analyzePeripherals(vector<u8>& rom, string hash) -> void {
  //J-Cart
  //======

  //Micro Machines: Military (Europe)
  if(hash == "43e68f99620bb3dd0ff3b28d232ae43c5b6489716c4cde09725ab4b71704937e") {
    peripherals.jcart = 1;
  }

  //Micro Machines: Turbo Tournament '96 (Europe) (1.0)
  if(hash == "ec3b97adf166b2e4e9ef402ac79ce9d458a932996b7fd259cd55db81adb496a6") {
    peripherals.jcart = 1;
  }

  //Micro Machines: Turbo Tournament '96 (Europe) (1.1)
  if(hash == "77ff023aa9f88aeb1605d3060b1331b816752b12b64ff2fe95d4a9f3c5dff946") {
    peripherals.jcart = 1;
  }

  //Micro Machines 2: Turbo Tournament (Europe) (1.0)
  if(hash == "57fa3662f589929d180648090104f8c802894602a6f660755121cb44918f382c") {
    peripherals.jcart = 1;
  }

  //Micro Machines 2: Turbo Tournament (Europe) (1.1)
  if(hash == "9209241472f0e78f23405bb265c7c108c25413d9a01ec1455f4e2d17d80c188c") {
    peripherals.jcart = 1;
  }

  //Pete Sampras Tennis (USA, Europe) (1.0)
  if(hash == "6e21848dd36d9a5843c73875f0a3eb59a23981d3d31a17c824664dc963ed7fa8") {
    peripherals.jcart = 1;
  }

  //Pete Sampras Tennis (USA, Europe) (1.1)
  if(hash == "f7aeb92a8f21b5dfd97ecfd907b8db49f0601dca1d32d78ee8f01e0b5c7d6fc7") {
    peripherals.jcart = 1;
  }

  //Pete Sampras Tennis (USA, Europe) (1.2)
  if(hash == "1a9e3daa0c6963754ab57ddd791b054989c89d89010f0d3ab846aec5842879a1") {
    peripherals.jcart = 1;
  }

  //Super Skidmarks (Europe) (Beta)
  if(hash == "b10ca5fb33eec060b5b21d4c1960a38f2fd1f0048ae88018d7c19fc340b26c31") {
    peripherals.jcart = 1;
  }

  //Super Skidmarks (Europe) (1.0)
  if(hash == "78705e4d3c91c8dc078fb61a08bf37c9c8e2f7ee9e9b13ddfe8279e54e6e9c6b") {
    peripherals.jcart = 1;
  }
}

auto MegaDrive::analyzeCopyProtection(vector<u8>& rom, string hash) -> void {
  //Super Bubble Bobble (Taiwan)
  if(hash == "e0a310c89961d781432715f71ce92d2d559fc272a7b46ea7b77383365b27ce21") {
    rom[0x0123e4] = 0x60;
    rom[0x0123e5] = 0x2a;
  }
}
