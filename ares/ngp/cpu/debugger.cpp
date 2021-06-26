auto CPU::Debugger::load(Node::Object parent) -> void {
  memory.ram = parent->append<Node::Debugger::Memory>("CPU RAM");
  memory.ram->setSize(self.ram.size());
  memory.ram->setRead([&](u32 address) -> u8 {
    return self.ram.read(address);
  });
  memory.ram->setWrite([&](u32 address, u8 data) -> void {
    return self.ram.write(address, data);
  });

  tracer.instruction = parent->append<Node::Debugger::Tracer::Instruction>("Instruction", "CPU");
  tracer.instruction->setAddressBits(24);

  tracer.interrupt = parent->append<Node::Debugger::Tracer::Notification>("Interrupt", "CPU");

  tracer.systemCall = parent->append<Node::Debugger::Tracer::Notification>("System Call", "CPU");

  tracer.io = parent->append<Node::Debugger::Tracer::Notification>("I/O", "CPU");

  vectors.resize(64);
  for(u32 vector : range(vectors.size())) {
    vectors[vector].byte(0) = system.bios.read(0xfe00 + vector * 4 + 0);
    vectors[vector].byte(1) = system.bios.read(0xfe00 + vector * 4 + 1);
    vectors[vector].byte(2) = system.bios.read(0xfe00 + vector * 4 + 2);
  }
}

auto CPU::Debugger::unload(Node::Object parent) -> void {
  parent->remove(memory.ram);
  parent->remove(tracer.instruction);
  parent->remove(tracer.interrupt);
  parent->remove(tracer.systemCall);
  parent->remove(tracer.io);
  memory.ram.reset();
  tracer.instruction.reset();
  tracer.interrupt.reset();
  tracer.systemCall.reset();
  tracer.io.reset();
}

auto CPU::Debugger::instruction() -> void {
  if(unlikely(tracer.systemCall->enabled())) {
    auto PC = self.TLCS900H::load(TLCS900H::PC);
    if(!PC) return;
    if(auto vectorID = vectors.find(PC)) {
      auto RA3  = self.TLCS900H::load(TLCS900H::RA3);
      auto RC3  = self.TLCS900H::load(TLCS900H::RC3);
      auto RB3  = self.TLCS900H::load(TLCS900H::RB3);
      auto QC3  = self.TLCS900H::load(TLCS900H::QC3);
      auto RD3  = self.TLCS900H::load(TLCS900H::RD3);
      auto XDE3 = self.TLCS900H::load(TLCS900H::XDE3);
      auto XHL3 = self.TLCS900H::load(TLCS900H::XHL3);

      string name;
      vector<string> args;
      switch(*vectorID) {
      case 0x00:
        name = "VECT_SHUTDOWN";
        break;
      case 0x01:
        name = "VECT_CLOCKGEARSET";
        args.append({"clockSpeed:0x", hex(RB3, 2L)});
        args.append({"clockRegeneration:0x", hex(RC3, 2L)});
        break;
      case 0x02:
        name = "VECT_RTCGET";
        args.append({"storageAddress:0x", hex(XHL3, 6L)});
        break;
    //case 0x03: break; //unknown
      case 0x04:
        name = "VECT_INTLVSET";
        args.append({"interruptLevel:0x", hex(RB3, 2L)});
        args.append({"interruptNumber:0x", hex(RC3, 2L)});
        break;
      case 0x05:
        name = "VECT_SYSFONTSET";
        args.append({"foreground:0x", hex(RA3 >> 0, 1L)});
        args.append({"background:0x", hex(RA3 >> 4, 1L)});
        break;
      case 0x06:
        name = "VECT_FLASHWRITE";
        args.append({"address:0x", hex(RA3, 2L)});
        args.append({"length:0x", hex(RC3 << 8, 4L)});
        args.append({"source:0x", hex(XHL3, 6L)});
        args.append({"target:0x", hex(XDE3, 6L)});
        break;
      case 0x07:
        name = "VECT_FLASHALLERS";
        args.append({"address:0x", hex(RA3, 2L)});
        break;
      case 0x08:
        name = "VECT_FLASHERS";
        args.append({"address:0x", hex(RA3, 2L)});
        args.append({"block:0x", hex(RB3, 2L)});
        break;
      case 0x09:
        name = "VECT_ALARMSET";
        args.append({"day:0x", hex(QC3, 2L)});
        args.append({"hour:0x", hex(RB3, 2L)});
        args.append({"minute:0x", hex(RC3, 2L)});
        break;
    //case 0x0a: //unknown
      case 0x0b:
        name = "VECT_ALARMDOWNSET";
        args.append({"day:0x", hex(QC3, 2L)});
        args.append({"hour:0x", hex(RB3, 2L)});
        args.append({"minute:0x", hex(RC3, 2L)});
        break;
    //case 0x0c: //unknown
      case 0x0d:
        name = "VECT_FLASHPROTECT";
        args.append({"address:0x", hex(RA3, 2L)});
        args.append({"block:0x", hex(RB3, 2L)});
        args.append({"type:0x", hex(RC3, 2L)});
        args.append({"blocks:0x", hex(RD3, 2L)});
        break;
      case 0x0e:
        name = "VECT_GETMODESET";
        args.append({"mode:0x", hex(RA3, 2L)});
        break;
    //case 0x0f: //unknown
      case 0x10:
        name = "VECT_COMINIT";
        break;
      case 0x11:
        name = "VECT_COMSENDSTART";
        break;
      case 0x12:
        name = "VECT_COMRECEIVESTART";
        break;
      case 0x13:
        name = "VECT_COMCREATEDATA";
        args.append({"data:0x", hex(RB3, 2L)});
        break;
      case 0x14:
        name = "VECT_COMGETDATA";
        break;
      case 0x15:
        name = "VECT_COMONRTS";
        break;
      case 0x16:
        name = "VECT_COMOFFRTS";
        break;
      case 0x17:
        name = "VECT_COMSENDSTATUS";
        break;
      case 0x18:
        name = "VECT_COMRECEIVESTATUS";
        break;
      case 0x19:
        name = "VECT_COMCREATEBUFDATA";
        args.append({"address:0x", hex(XHL3, 6L)});
        args.append({"size:0x", hex(RB3, 2L)});
        break;
      case 0x1a:
        name = "VECT_COMGETBUFDATA";
        args.append({"address:0x", hex(XHL3, 6L)});
        args.append({"size:0x", hex(RB3, 2L)});
        break;
      default:
        name = {"VECT_UNKNOWN[$", hex(*vectorID, 2L), "]"};
        break;
      }
      tracer.systemCall->notify(string{name, "(", args.merge(", "), ")"});
    }
  }

  if(unlikely(tracer.instruction->enabled())) {
    auto PC = self.TLCS900H::load(self.TLCS900H::PC);
    if(tracer.instruction->address(PC)) {
      tracer.instruction->notify(self.disassembleInstruction(), self.disassembleContext());
    }
  }
}

auto CPU::Debugger::interrupt(u8 vector) -> void {
  if(unlikely(tracer.interrupt->enabled())) {
    static const string name[64] = {
      "SWI0",    //0x00
      "SWI1",    //0x04
      "SWI2",    //0x08
      "SWI3",    //0x0c
      "SWI4",    //0x10
      "SWI5",    //0x14
      "SWI6",    //0x18
      "SWI7",    //0x1c
      "NMI",     //0x20
      "INTWD",   //0x24
      "INT0",    //0x28
      "INT4",    //0x2c
      "INT5",    //0x30
      "INT6",    //0x34
      "INT7",    //0x38
      "$3C",     //0x3c
      "INTT0",   //0x40
      "INTT1",   //0x44
      "INTT2",   //0x48
      "INTT3",   //0x4c
      "INTTR4",  //0x50
      "INTTR5",  //0x54
      "INTTR6",  //0x58
      "INTTR7",  //0x5c
      "INTRX0",  //0x60
      "INTTX0",  //0x64
      "INTRX1",  //0x68
      "INTTX1",  //0x6c
      "INTAD",   //0x70
      "INTTC0",  //0x74
      "INTTC1",  //0x78
      "INTTC2",  //0x7c
      "INTTC3",  //0x80
      "$84", "$88", "$8c",
      "$90", "$94", "$98", "$9c",
      "$a0", "$a4", "$a8", "$ac",
      "$b0", "$b4", "$b8", "$bc",
      "$c0", "$c4", "$c8", "$cc",
      "$d0", "$d4", "$d8", "$dc",
      "$e0", "$e4", "$e8", "$ec",
      "$f0", "$f4", "$f8", "$fc",
    };
    tracer.interrupt->notify({"IRQ (", name[vector >> 2], ")"});
  }
}

auto CPU::Debugger::readIO(u8 address, u8 data) -> void {
  if(unlikely(tracer.io->enabled())) {
    tracer.io->notify({"read  ", hex(address, 2L), " = ", hex(data, 2L)});
  }
}

auto CPU::Debugger::writeIO(u8 address, u8 data) -> void {
  if(unlikely(tracer.io->enabled())) {
    if(address == 0x6f) return;  //ignore watchdog writes (too frequent)
    tracer.io->notify({"write ", hex(address, 2L), " = ", hex(data, 2L)});
  }
}
