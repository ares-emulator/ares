struct PCEngine : Cartridge {
  auto name() -> string override { return "PC Engine"; }
  auto export(string location) -> vector<uint8_t> override;
  auto heuristics(vector<uint8_t>& data, string location) -> string override;
};

auto PCEngine::export(string location) -> vector<uint8_t> {
  vector<uint8_t> data;
  append(data, {location, "program.rom"});
  return data;
}

auto PCEngine::heuristics(vector<uint8_t>& data, string location) -> string {
  if((data.size() & 0x1fff) == 512) {
    //remove header if present
    memory::move(&data[0], &data[512], data.size() - 512);
    data.resize(data.size() - 512);
  }

  string digest = Hash::SHA256(data).digest();

  string board = "Linear";
  if(data.size() ==  0x60000) board = "Split";
  if(data.size() == 0x280000) board = "Banked";

  //Populous
  if(digest == "16a796634a2b3da417a775cb64ffa3322ad7baf901c288934d28df967f7977f6") board = "RAM";
  if(digest == "bfc2e6dd19f2cb642d3a7bd149b9a5eb5a8007f0f1c626eb62ded2a1f609814e") board = "RAM";

  //Ten no Koe Bank
  if(digest == "16d43e32b34ed40b0a59f4122370f4d973af33a6122c9ccbfab6bb21cf36f1b3") board = "RAM";

  //PC Engine
  if(digest == "afe9f27f91ac918348555b86298b4f984643eafa2773196f2c5441ea84f0c3bb") board = "System Card 1.00";
  if(digest == "909f2153e624b76392684434ba5aa3e394cbba0318a0dda350140cb61ce9bc49") board = "System Card 2.00";
  if(digest == "0deb13845c7e44ea78a25bbbe324afd60a0ec29ea5a4cf5780349f1598d24cd3") board = "System Card 2.10";
  if(digest == "e11527b3b96ce112a037138988ca72fd117a6b0779c2480d9e03eaebece3d9ce") board = "System Card 3.00";

  //TurboGrafx
  if(digest == "edba5be43803b180e1d64ca678c3f8bdbf07180c9e2a65a5db69ad635951e6cc") board = "System Card 2.00";
  if(digest == "cadac2725711b3c442bcf237b02f5a5210c96f17625c35fa58f009e0ed39e4db") board = "System Card 3.00";

  string s;
  s += "game\n";
  s +={"  sha256: ", digest, "\n"};
  s +={"  name:   ", Media::name(location), "\n"};
  s +={"  label:  ", Media::name(location), "\n"};
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
  if(board == "System Card 3.00") {
  s += "    memory\n";
  s += "      type: RAM\n";
  s += "      size: 0x30000\n";
  s += "      content: Work\n";
  s += "      volatile\n";
  }
  return s;
}
