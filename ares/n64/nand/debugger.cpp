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

auto NAND::Debugger::command(NAND::Command cmd, string desc) -> void {
  if(unlikely(tracer.io->enabled())) {
    string name;
    switch(cmd) {
      case NAND::Command::Read0: name = "Read0"; break;
      case NAND::Command::Read1: name = "Read1"; break;
      case NAND::Command::ReadSpare: name = "ReadSpare"; break;
      case NAND::Command::ReadID: name = "ReadID"; break;
      case NAND::Command::Reset: name = "Reset"; break;
      case NAND::Command::PageProgramC1: name = "PageProgramC1"; break;
      case NAND::Command::PageProgramC2: name = "PageProgramC2"; break;
      case NAND::Command::PageProgramDummyC2: name = "PageProgramDummyC2"; break;
      case NAND::Command::CopyBackProgramC2: name = "CopyBackProgramC2"; break;
      case NAND::Command::CopyBackProgramDummyC1: name = "CopyBackProgramDummyC1"; break;
      case NAND::Command::BlockEraseC1: name = "BlockEraseC1"; break;
      case NAND::Command::BlockEraseC2: name = "BlockEraseC2"; break;
      case NAND::Command::ReadStatus: name = "ReadStatus"; break;
      case NAND::Command::ReadStatusMultiplane: name = "ReadStatusMultiplane"; break;
      default: name = {"CMD(0x", hex(u8(cmd), 2L), ")"};
    }
    string message = {"NAND", self.num, " Command=", name, ", ", desc};
    tracer.io->notify(message);
  }
}
