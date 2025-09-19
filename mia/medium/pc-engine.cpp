struct PCEngine : Cartridge {
  auto name() -> string override { return "PC Engine"; }
  auto extensions() -> std::vector<string> override { return {"pce"}; }
  auto load(string location) -> LoadResult override;
  auto save(string location) -> bool override;
  auto analyze(std::vector<u8>& rom) -> string;
};

auto PCEngine::load(string location) -> LoadResult {
  std::vector<u8> rom;
  if(directory::exists(location)) {
    append(rom, {location, "program.rom"});
  } else if(file::exists(location)) {
    rom = Cartridge::read(location);
  }
  if(rom.empty()) return romNotFound;

  this->location = location;
  this->manifest = analyze(rom);
  auto document = BML::unserialize(manifest);
  if(!document) return couldNotParseManifest;

  pak = new vfs::directory;
  pak->setAttribute("title",  document["game/title"].string());
  pak->setAttribute("region", document["game/region"].string());
  pak->setAttribute("board",  document["game/board"].string());
  pak->append("manifest.bml", manifest);
  pak->append("program.rom",  rom);

  if(auto node = document["game/board/memory(type=RAM,content=Save)"]) {
    Medium::load(node, ".ram");
  }
  if(auto node = document["game/board/memory(type=RAM,content=Dynamic)"]) {
    Medium::load(node, ".dram");
  }
  if(auto node = document["game/board/memory(type=RAM,content=Work)"]) {
    Medium::load(node, ".wram");
  }

  return successful;
}

auto PCEngine::save(string location) -> bool {
  auto document = BML::unserialize(manifest);

  if(auto node = document["game/board/memory(type=RAM,content=Save)"]) {
    Medium::save(node, ".ram");
  }

  return true;
}

auto PCEngine::analyze(std::vector<u8>& data) -> string {
  if((data.size() & 0x1fff) == 512) {
    //remove header if present
    memory::move(&data[0], &data[512], data.size() - 512);
    data.resize(data.size() - 512);
  }

  string digest = Hash::SHA256(data).digest();
  string title = Medium::name(location);

  string region = "NTSC-U";
  string board = "Linear";
  if(data.size() ==  0x60000) board = "Split";
  if(data.size() == 0x280000) board = "Banked";

  //Kore ga Pro Yakyuu '89 (Japan)
  if(digest == "d23f4d610424a66fcdbf444a6e1a7ac3e0637471b215b1ba2691356102ede84a") title = "Kore ga Pro Yakyuu '89";

  //Kore ga Pro Yakyuu '90 (Japan)
  if(digest == "b3765d35ff78b8d97d057a55c7c9f8daaf36a527e9d144b681e39ae724d4338d") title = "Kore ga Pro Yakyuu '90";

  //Populous
  if(digest == "16a796634a2b3da417a775cb64ffa3322ad7baf901c288934d28df967f7977f6") board = "RAM", region = "NTSC-J";
  if(digest == "bfc2e6dd19f2cb642d3a7bd149b9a5eb5a8007f0f1c626eb62ded2a1f609814e") board = "RAM", region = "NTSC-J";

  //Ten no Koe Bank
  if(digest == "16d43e32b34ed40b0a59f4122370f4d973af33a6122c9ccbfab6bb21cf36f1b3") board = "RAM";

  //TV Sports Baseball (USA) (Proto)
  if(digest == "8e8294f197aa260271d59a5971369523bb6ff8d52bab0ae6b40044f3875e8c86") title = "TV Sports Baseball";

  //TV Sports Football (Japan)
  if(digest == "16f4b39a916e8db28f6ec4094d0612de82bb5ad40a30aceb8488752376b08d8f"
  //TV Sports Football (USA)
  || digest == "c8a438359f1a65c0f0a079b2c89274760b34a78e32244f11b677eca43c5f22e4") title = "TV Sports Football";

  //TV Sports Hockey (Japan)
  if(digest == "0c4605a3c5fe9c035dda9478cbe06bf92884198dac112e2060435c9469d0b5b1"
  //TV Sports Hockey (USA)
  || digest == "866b93e455a2ebe5afff9386b517e02d1feb03e72c276ad8e7e3423e48c9a061") title = "TV Sports Hockey";

  //Valkyrie no Densetsu
  if(digest == "633e6cea16faa32a135099f6002ef08f253781ea26a03add646f18fcfecc71ce") title = "Valkyrie no Densetsu";

  //Victory Run - Eikou no 13,000km (Japan)
  if(digest == "5e2bce8efbba82bd89efd571c3534360344797262a5762cdf253aa61e01523a1"
  //Victory Run (USA)
  || digest == "769aafaee88972586a77c2b4359ad837844a429a7e7b86ae36d25fcaf0266b20") title = "Victory Run";

  //PC Engine System Card 1.00
  if(digest == "afe9f27f91ac918348555b86298b4f984643eafa2773196f2c5441ea84f0c3bb") board = "System Card", region = "NTSC-J";

  //PC Engine System Card 2.00
  if(digest == "909f2153e624b76392684434ba5aa3e394cbba0318a0dda350140cb61ce9bc49") board = "System Card", region = "NTSC-J";

  //PC Engine System Card 2.10
  if(digest == "0deb13845c7e44ea78a25bbbe324afd60a0ec29ea5a4cf5780349f1598d24cd3") board = "System Card", region = "NTSC-J";

  //PC Engine Super System Card 3.00
  //PC Engine Arcade Card Duo
  //PC Engine Arcade Card Pro
  //note: because all three cards use the same ROM image, it is not possible to distinguish them here.
  //since the Arcade Card Pro is the most capable card, and is fully backward compatible, that is chosen here.
  if(digest == "e11527b3b96ce112a037138988ca72fd117a6b0779c2480d9e03eaebece3d9ce") board = "Arcade Card Pro", region = "NTSC-J";

  // PAC-N10
  if(digest == "????????????????????????????????????????????????????????????????") board = "Arcade Card Pro", region = "NTSC-J";

  // PAC-N1
  if(digest == "459325690a458baebd77495c91e37c4dddfdd542ba13a821ce954e5bb245627f") board = "Arcade Card Pro", region = "NTSC-J";

  // PCE-LP1
  if(digest == "3f43b3b577117d84002e99cb0baeb97b0d65b1d70b4adadc68817185c6a687f0") board = "Arcade Card Pro", region = "NTSC-J";

  //TurboGrafx System Card 2.00
  if(digest == "edba5be43803b180e1d64ca678c3f8bdbf07180c9e2a65a5db69ad635951e6cc") board = "System Card", region = "NTSC-U";

  //TurboGrafx Super System Card 3.00
  if(digest == "cadac2725711b3c442bcf237b02f5a5210c96f17625c35fa58f009e0ed39e4db") board = "Super System Card", region = "NTSC-U";

  // Games Express (Blue)
  if(digest == "4b86bb96a48a4ca8375fc0109631d0b1d64f255a03b01de70594d40788ba6c3d") board = "Games Express", region = "NTSC-J";

  // Games Express (Green)
  if(digest == "da173b20694c2b52087b099b8c44e471d3ee08a666c90d4afd997f8e1382add8") board = "Games Express", region = "NTSC-J";

  string s;
  s += "game\n";
  s +={"  sha256: ", digest, "\n"};
  s +={"  name:   ", Medium::name(location), "\n"};
  s +={"  title:  ", title, "\n"};
  s +={"  region: ", region, "\n"};
  s +={"  board:  ", board, "\n"};
  s += "    memory\n";
  s += "      type: ROM\n";
  s +={"      size: 0x", hex(data.size()), "\n"};
  s += "      content: Program\n";
  if(board == "RAM") {
  s += "    memory\n";
  s += "      type: RAM\n";
  s += "      size: 0x8000\n";
  s += "      content: Save\n";
  }
  if(board == "Super System Card" || board == "Arcade Card Pro") {
  s += "    memory\n";
  s += "      type: RAM\n";
  s += "      size: 0x30000\n";
  s += "      content: Work\n";
  s += "      volatile\n";
  }
  if(board == "Arcade Card Duo" || board == "Arcade Card Pro") {
  s += "    memory\n";
  s += "      type: RAM\n";
  s += "      size: 0x200000\n";
  s += "      content: Dynamic\n";
  s += "      volatile\n";
  }
  return s;
}
