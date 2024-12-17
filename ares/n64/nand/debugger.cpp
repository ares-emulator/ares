auto NAND::Debugger::load(Node::Object parent) -> void {
  string nand_name = {"NAND", self.num};
  string spare_name = {"Spare", self.num};
  tracer.io = parent->append<Node::Debugger::Tracer::Notification>("I/O", nand_name);
  memory.nand = parent->append<Node::Debugger::Memory>(nand_name);
  memory.spare = parent->append<Node::Debugger::Memory>(spare_name);

  memory.nand->setSize(0x4000000);
  memory.nand->setRead([&](u32 address) -> u8 {
    return self.data.read<Byte>(address);
  });
  memory.nand->setWrite([&](u32 address, u8 data) -> void {
    return;
  });

  memory.spare->setSize(0x10000);
  memory.spare->setRead([&](u32 address) -> u8 {
    return self.spare.read<Byte>(address);
  });
  memory.spare->setWrite([&](u32 address, u8 data) -> void {
    return;
  });
}

auto NAND::Debugger::unload(Node::Object parent) -> void {
  parent->remove(tracer.io);
  parent->remove(memory.nand);
  parent->remove(memory.spare);
  tracer.io.reset();
  memory.nand.reset();
  memory.spare.reset();
}

auto NAND::Debugger::command(Command cmd, string desc) -> void {
  static const vector<string> cmdNames = {
    "Read0",
    "Read1",
    "ReadSpare",
    "ReadID",
    "Reset",
    "PageProgramC1",
    "PageProgramC2",
    "PageProgramDummyC2",
    "CopyBackProgramC2",
    "CopyBackProgramDummyC1",
    "BlockEraseC1",
    "BlockEraseC2",
    "ReadStatus",
    "ReadStatusMultiplane",
  };

  if(unlikely(tracer.io->enabled())) {
    string nand_name = {"NAND", self.num};
    string name = cmdNames(u8(cmd), {"CMD(0x", hex(u8(cmd), 2L), ")"});
    string message = {nand_name, " Command=", name, ", ", desc};
    tracer.io->notify(message);
  }
}
