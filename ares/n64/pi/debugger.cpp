auto PI::Debugger::load(Node::Object parent) -> void {
  tracer.io = parent->append<Node::Debugger::Tracer::Notification>("I/O", "PI");
  if(system._BB()) {
    tracer.ide[0] = parent->append<Node::Debugger::Tracer::Notification>("IDE0", "PI");
    tracer.ide[1] = parent->append<Node::Debugger::Tracer::Notification>("IDE1", "PI");
    tracer.ide[2] = parent->append<Node::Debugger::Tracer::Notification>("IDE2", "PI");
    tracer.ide[3] = parent->append<Node::Debugger::Tracer::Notification>("IDE3", "PI");

    memory.buffer = parent->append<Node::Debugger::Memory>("PI Buffer");
    memory.buffer->setSize(0x4e0);
    memory.buffer->setRead([&](u32 address) -> u8 {
      return pi.bb_nand.buffer.read<Byte>(address);
    });
    memory.buffer->setWrite([&](u32 address, u8 data) -> void {
      return pi.bb_nand.buffer.write<Byte>(address, data);
    });

    properties.atb = parent->append<Node::Debugger::Properties>("PI ATB Config");
    properties.atb->setQuery([&] {
      string output;
      output.append("ATB Configuration\n");
      for (auto i : range(PI::BB_ATB::MaxEntries)) {
        output.append("ATB [", i,"]\n");
        output.append("    ivSource    : ", boolean(pi.bb_atb.entries[i].ivSource), "\n");
        output.append("    dmaEnable   : ", boolean(pi.bb_atb.entries[i].dmaEnable), "\n");
        output.append("    cpuEnable   : ", boolean(pi.bb_atb.entries[i].cpuEnable), "\n");
        output.append("    nandAddr    : ", hex(pi.bb_atb.entries[i].nandAddr, 8L), "\n");
        output.append("    maxOffset   : ", hex(pi.bb_atb.entries[i].maxOffset, 8L), "\n");
        output.append("    pbusAddress : ", hex(pi.bb_atb.pbusAddresses[i], 8L), "\n");
        output.append("    mask        : ", hex(pi.bb_atb.addressMasks[i], 8L), "\n");
      }
      return output;
    });
  }
}

auto PI::Debugger::ioBuffers(bool mode, u32 address, u32 data, const char *buffer) -> void {
  if(unlikely(tracer.io->enabled())) {
    string message;
    if(mode == Read) {
      message = {buffer, "[", hex(address, 8L), "]", " => ", hex(data, 8L)};
    }
    if(mode == Write) {
      message = {buffer, "[", hex(address, 8L), "]", " <= ", hex(data, 8L)};
    }
    tracer.io->notify(message);
  }
}

auto PI::Debugger::io(bool mode, u32 address, u32 data) -> void {
  static const vector<string> registerNames = {
    "PI_DRAM_ADDRESS",
    "PI_PBUS_ADDRESS",
    "PI_READ_LENGTH",
    "PI_WRITE_LENGTH",
    "PI_STATUS",
    "PI_BSD_DOM1_LAT",
    "PI_BSD_DOM1_PWD",
    "PI_BSD_DOM1_PGS",
    "PI_BSD_DOM1_RLS",
    "PI_BSD_DOM2_LAT",
    "PI_BSD_DOM2_PWD",
    "PI_BSD_DOM2_PGS",
    "PI_BSD_DOM2_RLS",
    "PI_UNKNOWN_34",
    "PI_UNKNOWN_38",
    "PI_UNKNOWN_3C",
    "PI_BB_ATB_UPPER",
    "PI_UNKNOWN_44",
    "PI_BB_NAND_CTRL",
    "PI_BB_NAND_CFG",
    "PI_BB_AES_CTRL",
    "PI_BB_ALLOWED_IO",
    "PI_BB_RD_LEN",
    "PI_BB_WR_LEN",
    "PI_BB_GPIO",
    "PI_UNKNOWN_64",
    "PI_UNKNOWN_68",
    "PI_UNKNOWN_6C",
    "PI_BB_NAND_ADDR",
  };

  if(unlikely(tracer.io->enabled())) {
    string message;
    string name = registerNames(address, {"PI_UNKNOWN(", hex(address), ")"});
    if(mode == Read) {
      message = {name.split("|").first(), " => ", hex(data, 8L)};
    }
    if(mode == Write) {
      message = {name.split("|").last(), " <= ", hex(data, 8L)};
    }
    tracer.io->notify(message);
  }
}

auto PI::Debugger::ide(bool mode, u2 which, u16 data) -> void {
  Node::Debugger::Tracer::Notification ide;
  switch(which) {
    case 0: { ide = tracer.ide[0]; } break;
    case 1: { ide = tracer.ide[1]; } break;
    case 2: { ide = tracer.ide[2]; } break;
    case 3: { ide = tracer.ide[3]; } break;
  }

  if(unlikely(ide->enabled())) {
    string message;
    string name = {"IDE", which};
    if(mode == Read) {
      message = {name, " => ", hex(data, 4L)};
    }
    if(mode == Write) {
      message = {name, " <= ", hex(data, 4L)};
    }
    ide->notify(message);
  }
}
