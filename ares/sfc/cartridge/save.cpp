auto Cartridge::saveCartridge() -> void {
  if(auto node = board["memory(type=RAM,content=Save)"]) saveRAM(node);
  if(auto node = board["processor(identifier=MCC)"]) saveMCC(node);
  if(auto node = board["processor(architecture=W65C816S)"]) saveSA1(node);
  if(auto node = board["processor(architecture=GSU)"]) saveSuperFX(node);
  if(auto node = board["processor(architecture=ARM6)"]) saveARMDSP(node);
  if(auto node = board["processor(architecture=HG51BS169)"]) saveHitachiDSP(node);
  if(auto node = board["processor(architecture=uPD7725)"]) saveuPD7725(node);
  if(auto node = board["processor(architecture=uPD96050)"]) saveuPD96050(node);
  if(auto node = board["rtc(manufacturer=Epson)"]) saveEpsonRTC(node);
  if(auto node = board["rtc(manufacturer=Sharp)"]) saveSharpRTC(node);
  if(auto node = board["processor(identifier=SPC7110)"]) saveSPC7110(node);
  if(auto node = board["processor(identifier=OBC1)"]) saveOBC1(node);
}

//

auto Cartridge::saveMemory(AbstractMemory& ram, Markup::Node node) -> void {
  string name;
  if(auto architecture = node["architecture"].string()) name.append(architecture, ".");
  name.append(node["content"].string(), ".");
  name.append(node["type"].string());
  name.downcase();

  if(auto fp = pak->write(name)) {
    fp->write({ram.data(), ram.size()});
  }
}

//memory(type=RAM,content=Save)
auto Cartridge::saveRAM(Markup::Node node) -> void {
  saveMemory(ram, node);
}

//processor(identifier=MCC)
auto Cartridge::saveMCC(Markup::Node node) -> void {
  if(auto mcu = node["mcu"]) {
    if(auto memory = mcu["memory(type=RAM,content=Download)"]) {
      saveMemory(mcc.psram, memory);
    }
  }
}

//processor(architecture=W65C816S)
auto Cartridge::saveSA1(Markup::Node node) -> void {
  if(auto memory = node["memory(type=RAM,content=Save)"]) {
    saveMemory(sa1.bwram, memory);
  }

  if(auto memory = node["memory(type=RAM,content=Internal)"]) {
    saveMemory(sa1.iram, memory);
  }
}

//processor(architecture=GSU)
auto Cartridge::saveSuperFX(Markup::Node node) -> void {
  if(auto memory = node["memory(type=RAM,content=Save)"]) {
    saveMemory(superfx.ram, memory);
  }
}

//processor(architecture=ARM6)
auto Cartridge::saveARMDSP(Markup::Node node) -> void {
  if(auto fp = pak->write("arm6.data.ram")) {
    for(u32 n : range(16 * 1024)) fp->write(armdsp.programRAM[n]);
  }
}

//processor(architecture=HG51BS169)
auto Cartridge::saveHitachiDSP(Markup::Node node) -> void {
  saveMemory(hitachidsp.ram, node["ram"]);

  if(auto memory = node["memory(type=RAM,content=Save)"]) {
    saveMemory(hitachidsp.ram, memory);
  }

  if(auto fp = pak->write("hg51bs169.data.ram")) {
    for(u32 n : range(3 * 1024)) fp->write(hitachidsp.dataRAM[n]);
  }
}

//processor(architecture=uPD7725)
auto Cartridge::saveuPD7725(Markup::Node node) -> void {
  if(auto fp = pak->write("upd7725.data.ram")) {
    for(u32 n : range(256)) fp->writel(necdsp.dataRAM[n], 2);
  }
}

//processor(architecture=uPD96050)
auto Cartridge::saveuPD96050(Markup::Node node) -> void {
  if(auto fp = pak->write("upd96050.data.ram")) {
    for(u32 n : range(2 * 1024)) fp->writel(necdsp.dataRAM[n], 2);
  }
}

//rtc(manufacturer=Epson)
auto Cartridge::saveEpsonRTC(Markup::Node node) -> void {
  if(auto fp = pak->write("time.rtc")) {
    n8 data[16];
    epsonrtc.save(data);
    fp->write({data, 16});
  }
}

//rtc(manufacturer=Sharp)
auto Cartridge::saveSharpRTC(Markup::Node node) -> void {
  if(auto fp = pak->write("time.rtc")) {
    n8 data[16];
    sharprtc.save(data);
    fp->write({data, 16});
  }
}

//processor(identifier=SPC7110)
auto Cartridge::saveSPC7110(Markup::Node node) -> void {
  if(auto memory = node["memory(type=RAM,content=Save)"]) {
    saveMemory(spc7110.ram, memory);
  }
}

//processor(identifier=OBC1)
auto Cartridge::saveOBC1(Markup::Node node) -> void {
  if(auto memory = node["memory(type=RAM,content=Save)"]) {
    saveMemory(obc1.ram, memory);
  }
}
