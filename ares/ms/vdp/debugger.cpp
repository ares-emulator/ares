auto VDP::Debugger::load(Node::Object parent) -> void {
  memory.vram = parent->append<Node::Debugger::Memory>("VDP VRAM");
  memory.vram->setSize(self.vram.size());
  memory.vram->setRead([&](u32 address) -> u8 {
    return self.vram[address];
  });
  memory.vram->setWrite([&](u32 address, u8 data) -> void {
    self.vram[address] = data;
  });

  memory.cram = parent->append<Node::Debugger::Memory>("VDP CRAM");
  memory.cram->setSize(self.cram.size());
  memory.cram->setRead([&](u32 address) -> u8 {
    return self.cram[address];
  });
  memory.cram->setWrite([&](u32 address, u8 data) -> void {
    self.cram[address] = data;
  });

  tracer.io = parent->append<Node::Debugger::Tracer::Notification>("I/O", "VDP");
}

auto VDP::Debugger::io(n4 register, n8 data) -> void {
  if(unlikely(tracer.io->enabled())) {
    static const string name[16] = {
      /* $0 */ "mode control 1",
      /* $1 */ "mode control 2",
      /* $2 */ "name table base address",
      /* $3 */ "color table base address",
      /* $4 */ "pattern table base address",
      /* $5 */ "sprite attribute table base address",
      /* $6 */ "sprite pattern table base address",
      /* $7 */ "backdrop color",
      /* $8 */ "horizontal scroll offset",
      /* $9 */ "vertical scroll offset",
      /* $a */ "line counter",
      /* $b */ "unused",
      /* $c */ "unused",
      /* $d */ "unused",
      /* $e */ "unused",
      /* $f */ "unused",
    };
    tracer.io->notify({
      "$", hex(register, 2L), " = #$", hex(data, 2L),
      " @ ", pad(self.vcounter(), 3L), ",", pad(self.hcounter(), 3L),
      " ", name[register]
    });
  }
}
