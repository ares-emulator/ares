auto Cartridge::loadBoard(string board) -> Markup::Node {
  string output;

  if(board.beginsWith("SNSP-")) board.replace("SNSP-", "SHVC-", 1L);
  if(board.beginsWith("MAXI-")) board.replace("MAXI-", "SHVC-", 1L);
  if(board.beginsWith("MJSC-")) board.replace("MJSC-", "SHVC-", 1L);
  if(board.beginsWith("EA-"  )) board.replace("EA-",   "SHVC-", 1L);
  if(board.beginsWith("WEI-" )) board.replace("WEI-",  "SHVC-", 1L);

  if(auto fp = platform->open(system.node, "boards.bml", File::Read, File::Required)) {
    auto document = BML::unserialize(fp->reads());
    for(auto leaf : document.find("board")) {
      auto id = leaf.text();
      bool matched = id == board;
      if(!matched && id.match("*(*)*")) {
        auto part = id.transform("()", "||").split("|");
        for(auto& revision : part(1).split(",")) {
          if(string{part(0), revision, part(2)} == board) matched = true;
        }
      }
      if(matched) return leaf;
    }
  }

  return {};
}

auto Cartridge::loadCartridge(Markup::Node node) -> void {
  board = node["board"];
  if(!board) board = loadBoard(information.board);

  auto region = information.region;
  if(region.endsWith("BRA")
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
    information.region = "NTSC";
  } else {
    information.region = "PAL";
  }

  if(auto node = board["memory(type=ROM,content=Program)"]) loadROM(node);
  if(auto node = board["memory(type=ROM,content=Expansion)"]) loadROM(node);  //todo: handle this better
  if(auto node = board["memory(type=RAM,content=Save)"]) loadRAM(node);
  if(auto node = board["processor(identifier=ICD)"]) loadICD(node);
  if(auto node = board["processor(identifier=MCC)"]) loadMCC(node);
  if(auto node = board["slot(type=BSMemory)"]) loadBSMemory(node);
  if(auto node = board["slot(type=SufamiTurbo)[0]"]) loadSufamiTurboA(node);
  if(auto node = board["slot(type=SufamiTurbo)[1]"]) loadSufamiTurboB(node);
  if(auto node = board["dip"]) loadDIP(node);
  if(auto node = board["processor(architecture=uPD78214)"]) loadCompetition(node);
  if(auto node = board["processor(architecture=W65C816S)"]) loadSA1(node);
  if(auto node = board["processor(architecture=GSU)"]) loadSuperFX(node);
  if(auto node = board["processor(architecture=ARM6)"]) loadARMDSP(node);
  if(auto node = board["processor(architecture=HG51BS169)"]) loadHitachiDSP(node, information.board.match("2DC*") ? 2 : 1);
  if(auto node = board["processor(architecture=uPD7725)"]) loaduPD7725(node);
  if(auto node = board["processor(architecture=uPD96050)"]) loaduPD96050(node);
  if(auto node = board["rtc(manufacturer=Epson)"]) loadEpsonRTC(node);
  if(auto node = board["rtc(manufacturer=Sharp)"]) loadSharpRTC(node);
  if(auto node = board["processor(identifier=SPC7110)"]) loadSPC7110(node);
  if(auto node = board["processor(identifier=SDD1)"]) loadSDD1(node);
  if(auto node = board["processor(identifier=OBC1)"]) loadOBC1(node);

  if(auto fp = platform->open(Cartridge::node, "msu1/data.rom", File::Read)) loadMSU1();
}

//

auto Cartridge::loadMemory(AbstractMemory& ram, Markup::Node node, bool required) -> void {
  if(auto memory = lookupMemory(node)) {
    ram.allocate(memory["size"].natural());
    if(memory["type"].text() == "RAM" && memory["volatile"]) return;
    if(memory["type"].text() == "RTC" && memory["volatile"]) return;
    string name{memory["content"].text(), ".", memory["type"].text()};
    if(auto architecture = memory["architecture"].text()) name.prepend(architecture, ".");
    name.downcase();
    if(auto fp = platform->open(Cartridge::node, name, File::Read, required)) {
      fp->read(ram.data(), min(fp->size(), ram.size()));
    }
  }
}

template<typename T>  //T = ReadableMemory, WritableMemory, ProtectableMemory
auto Cartridge::loadMap(Markup::Node map, T& memory) -> uint {
  auto address = map["address"].text();
  auto size = map["size"].natural();
  auto base = map["base"].natural();
  auto mask = map["mask"].natural();
  if(size == 0) size = memory.size();
  if(size == 0) return print("loadMap(): size=0\n"), 0;  //does this ever actually occur?
  return bus.map({&T::read, &memory}, {&T::write, &memory}, address, size, base, mask);
}

auto Cartridge::loadMap(
  Markup::Node map,
  const function<uint8 (uint24, uint8)>& reader,
  const function<void (uint24, uint8)>& writer
) -> uint {
  auto address = map["address"].text();
  auto size = map["size"].natural();
  auto base = map["base"].natural();
  auto mask = map["mask"].natural();
  return bus.map(reader, writer, address, size, base, mask);
}

//memory(type=ROM,content=Program)
auto Cartridge::loadROM(Markup::Node node) -> void {
  loadMemory(rom, node, File::Required);
  for(auto leaf : node.find("map")) loadMap(leaf, rom);
}

//memory(type=RAM,content=Save)
auto Cartridge::loadRAM(Markup::Node node) -> void {
  loadMemory(ram, node, File::Optional);
  for(auto leaf : node.find("map")) loadMap(leaf, ram);
}

//processor(identifier=ICD)
auto Cartridge::loadICD(Markup::Node node) -> void {
  has.GameBoySlot = true;
  has.ICD = true;

  icd.Revision = node["revision"].natural();
  if(auto oscillator = lookupOscillator()) {
    icd.Frequency = oscillator["frequency"].natural();
  } else {
    icd.Frequency = 0;
  }

  //Game Boy core loads data through ICD interface
  for(auto map : node.find("map")) {
    loadMap(map, {&ICD::readIO, &icd}, {&ICD::writeIO, &icd});
  }
}

//processor(identifier=MCC)
auto Cartridge::loadMCC(Markup::Node node) -> void {
  has.MCC = true;

  for(auto map : node.find("map")) {
    loadMap(map, {&MCC::read, &mcc}, {&MCC::write, &mcc});
  }

  if(auto mcu = node["mcu"]) {
    for(auto map : mcu.find("map")) {
      loadMap(map, {&MCC::mcuRead, &mcc}, {&MCC::mcuWrite, &mcc});
    }
    if(auto memory = mcu["memory(type=ROM,content=Program)"]) {
      loadMemory(mcc.rom, memory, File::Required);
    }
    if(auto memory = mcu["memory(type=RAM,content=Download)"]) {
      loadMemory(mcc.psram, memory, File::Optional);
    }
    if(auto slot = mcu["slot(type=BSMemory)"]) {
      loadBSMemory(slot);
    }
  }
}

//slot(type=BSMemory)
auto Cartridge::loadBSMemory(Markup::Node node) -> void {
  has.BSMemorySlot = true;

  if(auto node = board["slot(type=BSMemory)"]) {
    for(auto map : node.find("map")) {
      loadMap(map, {&BSMemoryCartridge::read, &bsmemory}, {&BSMemoryCartridge::write, &bsmemory});
    }
  }
}

//slot(type=SufamiTurbo)[0]
auto Cartridge::loadSufamiTurboA(Markup::Node node) -> void {
  has.SufamiTurboSlotA = true;

  for(auto map : node.find("rom/map")) {
    loadMap(map, {&SufamiTurboCartridge::readROM, &sufamiturboA}, {&SufamiTurboCartridge::writeROM, &sufamiturboA});
  }

  for(auto map : node.find("ram/map")) {
    loadMap(map, {&SufamiTurboCartridge::readRAM, &sufamiturboA}, {&SufamiTurboCartridge::writeRAM, &sufamiturboA});
  }
}

//slot(type=SufamiTurbo)[1]
auto Cartridge::loadSufamiTurboB(Markup::Node node) -> void {
  has.SufamiTurboSlotB = true;

  for(auto map : node.find("rom/map")) {
    loadMap(map, {&SufamiTurboCartridge::readROM, &sufamiturboB}, {&SufamiTurboCartridge::writeROM, &sufamiturboB});
  }

  for(auto map : node.find("ram/map")) {
    loadMap(map, {&SufamiTurboCartridge::readROM, &sufamiturboB}, {&SufamiTurboCartridge::writeROM, &sufamiturboB});
  }
}

//dip
auto Cartridge::loadDIP(Markup::Node node) -> void {
  has.DIP = true;
  dip.value = 0;  //todo

  for(auto map : node.find("map")) {
    loadMap(map, {&DIP::read, &dip}, {&DIP::write, &dip});
  }
}

//processor(architecture=uPD78214)
auto Cartridge::loadCompetition(Markup::Node node) -> void {
  has.Competition = true;
  competition.board = Competition::Board::Unknown;
  if(node["identifier"].text() == "Campus Challenge '92") competition.board = Competition::Board::CampusChallenge92;
  if(node["identifier"].text() == "PowerFest '94") competition.board = Competition::Board::PowerFest94;

  for(auto map : node.find("map")) {
    loadMap(map, {&Competition::read, &competition}, {&Competition::write, &competition});
  }

  if(auto mcu = node["mcu"]) {
    for(auto map : mcu.find("map")) {
      loadMap(map, {&Competition::mcuRead, &competition}, {&Competition::mcuWrite, &competition});
    }
    if(auto memory = mcu["memory(type=ROM,content=Program)"]) {
      loadMemory(competition.rom[0], memory, File::Required);
    }
    if(auto memory = mcu["memory(type=ROM,content=Level-1)"]) {
      loadMemory(competition.rom[1], memory, File::Required);
    }
    if(auto memory = mcu["memory(type=ROM,content=Level-2)"]) {
      loadMemory(competition.rom[2], memory, File::Required);
    }
    if(auto memory = mcu["memory(type=ROM,content=Level-3)"]) {
      loadMemory(competition.rom[3], memory, File::Required);
    }
  }
}

//processor(architecture=W65C816S)
auto Cartridge::loadSA1(Markup::Node node) -> void {
  has.SA1 = true;

  for(auto map : node.find("map")) {
    loadMap(map, {&SA1::readIOCPU, &sa1}, {&SA1::writeIOCPU, &sa1});
  }

  if(auto mcu = node["mcu"]) {
    for(auto map : mcu.find("map")) {
      loadMap(map, {&SA1::ROM::readCPU, &sa1.rom}, {&SA1::ROM::writeCPU, &sa1.rom});
    }
    if(auto memory = mcu["memory(type=ROM,content=Program)"]) {
      loadMemory(sa1.rom, memory, File::Required);
    }
    if(auto slot = mcu["slot(type=BSMemory)"]) {
      loadBSMemory(slot);
    }
  }

  if(auto memory = node["memory(type=RAM,content=Save)"]) {
    loadMemory(sa1.bwram, memory, File::Optional);
    for(auto map : memory.find("map")) {
      loadMap(map, {&SA1::BWRAM::readCPU, &sa1.bwram}, {&SA1::BWRAM::writeCPU, &sa1.bwram});
    }
  }

  if(auto memory = node["memory(type=RAM,content=Internal)"]) {
    loadMemory(sa1.iram, memory, File::Optional);
    for(auto map : memory.find("map")) {
      loadMap(map, {&SA1::IRAM::readCPU, &sa1.iram}, {&SA1::IRAM::writeCPU, &sa1.iram});
    }
  }
}

//processor(architecture=GSU)
auto Cartridge::loadSuperFX(Markup::Node node) -> void {
  has.SuperFX = true;

  if(auto oscillator = lookupOscillator()) {
    superfx.Frequency = oscillator["frequency"].natural();  //GSU-1, GSU-2
  } else {
    superfx.Frequency = system.cpuFrequency();  //MARIO CHIP 1
  }

  for(auto map : node.find("map")) {
    loadMap(map, {&SuperFX::readIO, &superfx}, {&SuperFX::writeIO, &superfx});
  }

  if(auto memory = node["memory(type=ROM,content=Program)"]) {
    loadMemory(superfx.rom, memory, File::Required);
    for(auto map : memory.find("map")) {
      loadMap(map, superfx.cpurom);
    }
  }

  if(auto memory = node["memory(type=RAM,content=Save)"]) {
    loadMemory(superfx.ram, memory, File::Optional);
    for(auto map : memory.find("map")) {
      loadMap(map, superfx.cpuram);
    }
  }

  if(auto memory = node["memory(type=RAM,content=Backup)"]) {
    loadMemory(superfx.bram, memory, File::Optional);
    for(auto map : memory.find("map")) {
      loadMap(map, superfx.cpubram);
    }
  }
}

//processor(architecture=ARM6)
auto Cartridge::loadARMDSP(Markup::Node node) -> void {
  has.ARMDSP = true;

  for(auto& word : armdsp.programROM) word = 0x00;
  for(auto& word : armdsp.dataROM) word = 0x00;
  for(auto& word : armdsp.programRAM) word = 0x00;

  if(auto oscillator = lookupOscillator()) {
    armdsp.Frequency = oscillator["frequency"].natural();
  } else {
    armdsp.Frequency = 21'440'000;
  }

  for(auto map : node.find("map")) {
    loadMap(map, {&ARMDSP::read, &armdsp}, {&ARMDSP::write, &armdsp});
  }

  if(auto memory = node["memory(type=ROM,content=Program,architecture=ARM6)"]) {
    if(auto fp = platform->open(Cartridge::node, "arm6.program.rom", File::Read, File::Required)) {
      for(uint n : range(128 * 1024)) armdsp.programROM[n] = fp->read();
    }
  }

  if(auto memory = node["memory(type=ROM,content=Data,architecture=ARM6)"]) {
    if(auto fp = platform->open(Cartridge::node, "arm6.data.rom", File::Read, File::Required)) {
      for(uint n : range(32 * 1024)) armdsp.dataROM[n] = fp->read();
    }
  }

  if(auto memory = node["memory(type=RAM,content=Data,architecture=ARM6)"]) {
    if(auto fp = platform->open(Cartridge::node, "arm6.data.ram", File::Read)) {
      for(uint n : range(16 * 1024)) armdsp.programRAM[n] = fp->read();
    }
  }
}

//processor(architecture=HG51BS169)
auto Cartridge::loadHitachiDSP(Markup::Node node, uint roms) -> void {
  has.HitachiDSP = true;

  for(auto& word : hitachidsp.dataROM) word = 0x000000;
  for(auto& word : hitachidsp.dataRAM) word = 0x00;

  if(auto oscillator = lookupOscillator()) {
    hitachidsp.Frequency = oscillator["frequency"].natural();
  } else {
    hitachidsp.Frequency = 20'000'000;
  }
  hitachidsp.Roms = roms;  //1 or 2
  hitachidsp.Mapping = 0;  //0 or 1

  for(auto map : node.find("map")) {
    loadMap(map, {&HitachiDSP::readIO, &hitachidsp}, {&HitachiDSP::writeIO, &hitachidsp});
  }

  if(auto memory = node["memory(type=ROM,content=Program)"]) {
    loadMemory(hitachidsp.rom, memory, File::Required);
    for(auto map : memory.find("map")) {
      loadMap(map, {&HitachiDSP::readROM, &hitachidsp}, {&HitachiDSP::writeROM, &hitachidsp});
    }
  }

  if(auto memory = node["memory(type=RAM,content=Save)"]) {
    loadMemory(hitachidsp.ram, memory, File::Optional);
    for(auto map : memory.find("map")) {
      loadMap(map, {&HitachiDSP::readRAM, &hitachidsp}, {&HitachiDSP::writeRAM, &hitachidsp});
    }
  }

  if(auto memory = node["memory(type=ROM,content=Data,architecture=HG51BS169)"]) {
    if(auto fp = platform->open(Cartridge::node, "hg51bs169.data.rom", File::Read, File::Required)) {
      for(uint n : range(1 * 1024)) hitachidsp.dataROM[n] = fp->readl(3);
    }
  }

  if(auto memory = node["memory(type=RAM,content=Data,architecture=HG51BS169)"]) {
    if(auto fp = platform->open(Cartridge::node, "hg51bs169.data.ram", File::Read)) {
      for(uint n : range(3 * 1024)) hitachidsp.dataRAM[n] = fp->readl(1);
    }
    for(auto map : memory.find("map")) {
      loadMap(map, {&HitachiDSP::readDRAM, &hitachidsp}, {&HitachiDSP::writeDRAM, &hitachidsp});
    }
  }
}

//processor(architecture=uPD7725)
auto Cartridge::loaduPD7725(Markup::Node node) -> void {
  has.NECDSP = true;
  necdsp.revision = NECDSP::Revision::uPD7725;

  for(auto& word : necdsp.programROM) word = 0x000000;
  for(auto& word : necdsp.dataROM) word = 0x0000;
  for(auto& word : necdsp.dataRAM) word = 0x0000;

  if(auto oscillator = lookupOscillator()) {
    necdsp.Frequency = oscillator["frequency"].natural();
  } else {
    necdsp.Frequency = 7'600'000;
  }

  for(auto map : node.find("map")) {
    loadMap(map, {&NECDSP::read, &necdsp}, {&NECDSP::write, &necdsp});
  }

  if(auto memory = node["memory(type=ROM,content=Program,architecture=uPD7725)"]) {
    if(auto fp = platform->open(Cartridge::node, "upd7725.program.rom", File::Read, File::Required)) {
      for(uint n : range(2048)) necdsp.programROM[n] = fp->readl(3);
    }
  }

  if(auto memory = node["memory(type=ROM,content=Data,architecture=uPD7725)"]) {
    if(auto fp = platform->open(Cartridge::node, "upd7725.data.rom", File::Read, File::Required)) {
      for(uint n : range(1024)) necdsp.dataROM[n] = fp->readl(2);
    }
  }

  if(auto memory = node["memory(type=RAM,content=Data,architecture=uPD7725)"]) {
    if(auto fp = platform->open(Cartridge::node, "upd7725.data.ram", File::Read)) {
      for(uint n : range(256)) necdsp.dataRAM[n] = fp->readl(2);
    }
    for(auto map : memory.find("map")) {
      loadMap(map, {&NECDSP::readRAM, &necdsp}, {&NECDSP::writeRAM, &necdsp});
    }
  }
}

//processor(architecture=uPD96050)
auto Cartridge::loaduPD96050(Markup::Node node) -> void {
  has.NECDSP = true;
  necdsp.revision = NECDSP::Revision::uPD96050;

  for(auto& word : necdsp.programROM) word = 0x000000;
  for(auto& word : necdsp.dataROM) word = 0x0000;
  for(auto& word : necdsp.dataRAM) word = 0x0000;

  if(auto oscillator = lookupOscillator()) {
    necdsp.Frequency = oscillator["frequency"].natural();
  } else {
    necdsp.Frequency = 11'000'000;
  }

  for(auto map : node.find("map")) {
    loadMap(map, {&NECDSP::read, &necdsp}, {&NECDSP::write, &necdsp});
  }

  if(auto memory = node["memory(type=ROM,content=Program,architecture=uPD96050)"]) {
    if(auto fp = platform->open(Cartridge::node, "upd96050.program.rom", File::Read, File::Required)) {
      for(uint n : range(16384)) necdsp.programROM[n] = fp->readl(3);
    }
  }

  if(auto memory = node["memory(type=ROM,content=Data,architecture=uPD96050)"]) {
    if(auto fp = platform->open(Cartridge::node, "upd96050.data.rom", File::Read, File::Required)) {
      for(uint n : range(2048)) necdsp.dataROM[n] = fp->readl(2);
    }
  }

  if(auto memory = node["memory(type=RAM,content=Data,architecture=uPD96050)"]) {
    if(auto fp = platform->open(Cartridge::node, "upd96050.data.ram", File::Read)) {
      for(uint n : range(2048)) necdsp.dataRAM[n] = fp->readl(2);
    }
    for(auto map : memory.find("map")) {
      loadMap(map, {&NECDSP::readRAM, &necdsp}, {&NECDSP::writeRAM, &necdsp});
    }
  }
}

//rtc(manufacturer=Epson)
auto Cartridge::loadEpsonRTC(Markup::Node node) -> void {
  has.EpsonRTC = true;

  epsonrtc.initialize();

  for(auto map : node.find("map")) {
    loadMap(map, {&EpsonRTC::read, &epsonrtc}, {&EpsonRTC::write, &epsonrtc});
  }

  if(auto memory = node["memory(type=RTC,content=Time,manufacturer=Epson)"]) {
    if(auto fp = platform->open(Cartridge::node, "epson.time.rtc", File::Read)) {
      uint8 data[16] = {0};
      for(auto& byte : data) byte = fp->read();
      epsonrtc.load(data);
    }
  }
}

//rtc(manufacturer=Sharp)
auto Cartridge::loadSharpRTC(Markup::Node node) -> void {
  has.SharpRTC = true;

  sharprtc.initialize();

  for(auto map : node.find("map")) {
    loadMap(map, {&SharpRTC::read, &sharprtc}, {&SharpRTC::write, &sharprtc});
  }

  if(auto memory = node["memory(type=RTC,content=Time,manufacturer=Sharp)"]) {
    if(auto fp = platform->open(Cartridge::node, "sharp.time.rtc", File::Read)) {
      uint8 data[16] = {0};
      for(auto& byte : data) byte = fp->read();
      sharprtc.load(data);
    }
  }
}

//processor(identifier=SPC7110)
auto Cartridge::loadSPC7110(Markup::Node node) -> void {
  has.SPC7110 = true;

  for(auto map : node.find("map")) {
    loadMap(map, {&SPC7110::read, &spc7110}, {&SPC7110::write, &spc7110});
  }

  if(auto mcu = node["mcu"]) {
    for(auto map : mcu.find("map")) {
      loadMap(map, {&SPC7110::mcuromRead, &spc7110}, {&SPC7110::mcuromWrite, &spc7110});
    }
    if(auto memory = mcu["memory(type=ROM,content=Program)"]) {
      loadMemory(spc7110.prom, memory, File::Required);
    }
    if(auto memory = mcu["memory(type=ROM,content=Data)"]) {
      loadMemory(spc7110.drom, memory, File::Required);
    }
  }

  if(auto memory = node["memory(type=RAM,content=Save)"]) {
    loadMemory(spc7110.ram, memory, File::Optional);
    for(auto map : memory.find("map")) {
      loadMap(map, {&SPC7110::mcuramRead, &spc7110}, {&SPC7110::mcuramWrite, &spc7110});
    }
  }
}

//processor(identifier=SDD1)
auto Cartridge::loadSDD1(Markup::Node node) -> void {
  has.SDD1 = true;

  for(auto map : node.find("map")) {
    loadMap(map, {&SDD1::ioRead, &sdd1}, {&SDD1::ioWrite, &sdd1});
  }

  if(auto mcu = node["mcu"]) {
    for(auto map : mcu.find("map")) {
      loadMap(map, {&SDD1::mcuRead, &sdd1}, {&SDD1::mcuWrite, &sdd1});
    }
    if(auto memory = mcu["memory(type=ROM,content=Program)"]) {
      loadMemory(sdd1.rom, memory, File::Required);
    }
  }
}

//processor(identifier=OBC1)
auto Cartridge::loadOBC1(Markup::Node node) -> void {
  has.OBC1 = true;

  for(auto map : node.find("map")) {
    loadMap(map, {&OBC1::read, &obc1}, {&OBC1::write, &obc1});
  }

  if(auto memory = node["memory(type=RAM,content=Save)"]) {
    loadMemory(obc1.ram, memory, File::Optional);
  }
}

//file::exists("msu1/data.rom")
auto Cartridge::loadMSU1() -> void {
  has.MSU1 = true;

  bus.map({&MSU1::readIO, &msu1}, {&MSU1::writeIO, &msu1}, "00-3f,80-bf:2000-2007");
}
