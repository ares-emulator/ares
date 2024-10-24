NALL_NOINLINE auto TLCS900H::disassembleInstruction() -> string {
  string output;

  auto pc = load(PC);

  n8 op[8], ops;
  auto read8 = [&]() -> n8 {
    return op[ops++] = disassembleRead(pc++);
  };
  auto read16 = [&]() -> n16 {
    n8 d0 = read8();
    n8 d1 = read8();
    return d0 << 0 | d1 << 8;
  };
  auto read24 = [&]() -> n24 {
    n8 d0 = read8();
    n8 d1 = read8();
    n8 d2 = read8();
    return d0 << 0 | d1 << 8 | d2 << 16;
  };
  auto read32 = [&]() -> n32 {
    n8 d0 = read8();
    n8 d1 = read8();
    n8 d2 = read8();
    n8 d3 = read8();
    return d0 << 0 | d1 << 8 | d2 << 16 | d3 << 24;
  };

  enum : u32 {
    Null,
    Text,                            //text
    Condition,                       //condition
    Register,                        //register.size
    Control,                         //register.size
    Immediate,                       //immediate.size
    Displacement,                    //displacement.size
    DisplacementPC,                  //pc+displacement.size
    IndirectRegister,                //op.size (r32)
    IndirectRegisterDecrement,       //op.size (-r32)
    IndirectRegisterIncrement,       //op.size (r32+)
    IndirectRegisterRegister8,       //op.size (r32+r8)
    IndirectRegisterRegister16,      //op.size (r32+r16)
    IndirectRegisterDisplacement8,   //op.size (r32+d8)
    IndirectRegisterDisplacement16,  //op.size (r32+d16)
    IndirectImmediate8,              //(i8)
    IndirectImmediate16,             //(i16)
    IndirectImmediate24,             //(i24)
  };

  //name [lhs[,rhs]]
  string name;
  struct Operand {
    explicit operator bool() const { return _mode != Null; }

    auto mode() const { return _mode; }
    auto size() const { return _size; }
    auto text() const { return _text; }
    auto condition() const { return _condition; }
    auto register() const { return _register; }
    auto registerAdd() const { return _registerAdd; }
    auto immediate() const { return _immediate; }
    auto displacement() const { return _displacement; }

    auto mode(natural mode_) -> void { _mode = mode_; }
    auto size(natural size_) -> void { _size = size_; }

    auto null() -> void { mode(Null); }
    auto text(string text_) -> void { mode(Text); _text = text_; }
    auto condition(n4 condition_) -> void { mode(Condition); _condition = condition_; }
    auto register(natural size_, n8 register_) -> void { mode(Register); size(size_); _register = register_; }
    auto register3(natural size_, n3 register_) -> void { mode(Register); size(size_); _register = lookup(size_, register_); }
    auto control(natural size_, n8 register_) -> void { mode(Control); size(size_); _register = register_; }
    auto immediate(natural size_, natural immediate_) -> void { mode(Immediate); size(size_); _immediate = immediate_; }
    auto displacement(natural size_, natural displacement_) -> void { mode(Displacement); size(size_); _displacement = displacement_; }
    auto displacementPC(natural size_, natural displacement_) -> void { mode(DisplacementPC); size(size_); _displacement = displacement_; }

    auto indirectRegister(natural size_, n8 register_) -> void { mode(IndirectRegister); size(size_); _register = register_; }
    auto indirectRegister3(natural size_, n3 register_) -> void { mode(IndirectRegister); size(size_); _register = lookup(32, register_); }
    auto indirectRegisterDecrement(natural size_, n8 register_) -> void { mode(IndirectRegisterDecrement); size(size_); _register = register_; }
    auto indirectRegisterIncrement(natural size_, n8 register_) -> void { mode(IndirectRegisterIncrement); size(size_); _register = register_; }
    auto indirectRegister3Increment(natural size_, n3 register_) -> void { mode(IndirectRegisterIncrement); size(size_); _register = lookup(32, register_); }
    auto indirectRegisterRegister8(natural size_, n8 register32, n8 register8) -> void { mode(IndirectRegisterRegister8); size(size_); _register = register32; _registerAdd = register8; }
    auto indirectRegisterRegister16(natural size_, n8 register32, n8 register16) -> void { mode(IndirectRegisterRegister16); size(size_); _register = register32; _registerAdd = register16; }
    auto indirectRegister3Displacement8(natural size_, n3 register_, n8 displacement_) -> void { mode(IndirectRegisterDisplacement8); size(size_); _register = lookup(32, register_); _displacement = displacement_; }
    auto indirectRegisterDisplacement16(natural size_, n8 register_, n16 displacement_) -> void { mode(IndirectRegisterDisplacement16); size(size_); _register = register_; _displacement = displacement_; }
    auto indirectImmediate8(natural size_, n8 immediate_) -> void { mode(IndirectImmediate8); size(size_); _immediate = immediate_; }
    auto indirectImmediate16(natural size_, n16 immediate_) -> void { mode(IndirectImmediate16); size(size_); _immediate = immediate_; }
    auto indirectImmediate24(natural size_, n24 immediate_) -> void { mode(IndirectImmediate24); size(size_); _immediate = immediate_; }

  private:
    natural _mode = Null;
    natural _size;
    string  _text;
    natural _condition;
    natural _register;
    natural _registerAdd;
    natural _immediate;
    natural _displacement;

    static auto lookup(natural size, n3 register) -> n8 {
      if(size ==  8) return from_array<0xe1,0xe0,0xe5,0xe4,0xe9,0xe8,0xed,0xec>(register);
      if(size == 16) return from_array<0xe0,0xe4,0xe8,0xec,0xf0,0xf4,0xf8,0xfc>(register);
      if(size == 32) return from_array<0xe0,0xe4,0xe8,0xec,0xf0,0xf4,0xf8,0xfc>(register);
      return 0;
    }
  } lhs, rhs;

  boolean opRegister;
  boolean opSourceMemory;
  boolean opTargetMemory;

  static const natural opSizes[] = {8, 16, 32, 0};  //0xc0 - 0xf5 use combined logic switch cases:
  #define opSize opSizes[fetch.bit(4,5)]            //extract the size from the opcode fetch
  switch(auto fetch = read8()) {
  case 0x00: name = "nop"; break;
  case 0x01: break;  //"normal" (not present on 900/H)
  case 0x02: name = "push"; lhs.text("sr"); break;
  case 0x03: name = "pop"; lhs.text("sr"); break;
  case 0x04: break;  //"max" or "min" (not present on 900/H)
  case 0x05: name = "halt"; break;
  case 0x06: name = "ei"; lhs.immediate(3, (n3)read8()); if(lhs.immediate() == 7) name = "di", lhs.null(); break;
  case 0x07: name = "reti"; break;
  case 0x08: name = "ld"; lhs.indirectImmediate8(8, read8()); rhs.immediate(8, read8()); break;
  case 0x09: name = "push"; lhs.immediate(8, read8()); break;
  case 0x0a: name = "ldw"; lhs.indirectImmediate8(8, read8()); rhs.immediate(16, read16()); break;
  case 0x0b: name = "pushw"; lhs.immediate(16, read16()); break;
  case 0x0c: name = "incf"; break;
  case 0x0d: name = "decf"; break;
  case 0x0e: name = "ret"; break;
  case 0x0f: name = "retd"; lhs.displacement(16, read16()); break;
  case 0x10: name = "rcf"; break;
  case 0x11: name = "scf"; break;
  case 0x12: name = "ccf"; break;
  case 0x13: name = "zcf"; break;
  case 0x14: name = "push"; lhs.register3(8, A.id); break;
  case 0x15: name = "pop"; lhs.register3(8, A.id); break;
  case 0x16: name = "ex"; lhs.text("f"); rhs.text("f'"); break;
  case 0x17: name = "ldf"; lhs.immediate(2, (n2)read8()); break;
  case 0x18: name = "push"; lhs.text("f"); break;
  case 0x19: name = "pop"; lhs.text("f"); break;
  case 0x1a: name = "jp"; lhs.immediate(16, read16()); break;
  case 0x1b: name = "jp"; lhs.immediate(24, read24()); break;
  case 0x1c: name = "call"; lhs.immediate(16, read16()); break;
  case 0x1d: name = "call"; lhs.immediate(24, read24()); break;
  case 0x1e: name = "calr"; lhs.displacementPC(16, read16()); break;
  case 0x1f: break;
  case range8(0x20, 0x27): name = "ld"; lhs.register3(8, fetch); rhs.immediate(8, read8()); break;
  case range8(0x28, 0x2f): name = "push"; lhs.register3(16, fetch); break;
  case range8(0x30, 0x37): name = "ld"; lhs.register3(16, fetch); rhs.immediate(16, read16()); break;
  case range8(0x38, 0x3f): name = "push"; lhs.register3(32, fetch); break;
  case range8(0x40, 0x47): name = "ld"; lhs.register3(32, fetch); rhs.immediate(32, read32()); break;
  case range8(0x48, 0x4f): name = "pop"; lhs.register3(16, fetch); break;
  case range16(0x50, 0x5f): name = "pop"; lhs.register3(32, fetch); break;
  case range16(0x60, 0x6f): name = "jr"; lhs.condition(fetch); rhs.displacementPC(8, read8()); break;
  case range16(0x70, 0x7f): name = "jrl"; lhs.condition(fetch); rhs.displacementPC(16, read16()); break;
  case range8(0x80, 0x87): opSourceMemory = true; lhs.indirectRegister3(8, fetch); break;
  case range8(0x88, 0x8f): opSourceMemory = true; lhs.indirectRegister3Displacement8(8, fetch, read8()); break;
  case range8(0x90, 0x97): opSourceMemory = true; lhs.indirectRegister3(16, fetch); break;
  case range8(0x98, 0x9f): opSourceMemory = true; lhs.indirectRegister3Displacement8(16, fetch, read8()); break;
  case range8(0xa0, 0xa7): opSourceMemory = true; lhs.indirectRegister3(32, fetch); break;
  case range8(0xa8, 0xaf): opSourceMemory = true; lhs.indirectRegister3Displacement8(32, fetch, read8()); break;
  case range8(0xb0, 0xb7): opTargetMemory = true; lhs.indirectRegister3(0, fetch); break;
  case range8(0xb8, 0xbf): opTargetMemory = true; lhs.indirectRegister3Displacement8(0, fetch, read8()); break;
  case 0xc0: case 0xd0: case 0xe0: case 0xf0:
    opSourceMemory = fetch < 0xf0; opTargetMemory = !opSourceMemory;
    lhs.indirectImmediate8(opSize, read8()); break;
  case 0xc1: case 0xd1: case 0xe1: case 0xf1:
    opSourceMemory = fetch < 0xf0; opTargetMemory = !opSourceMemory;
    lhs.indirectImmediate16(opSize, read16()); break;
  case 0xc2: case 0xd2: case 0xe2: case 0xf2:
    opSourceMemory = fetch < 0xf0; opTargetMemory = !opSourceMemory;
    lhs.indirectImmediate24(opSize, read24()); break;
  case 0xc3: case 0xd3: case 0xe3: case 0xf3: {
    opSourceMemory = fetch < 0xf0; opTargetMemory = !opSourceMemory;
    auto data = read8();
    if((n2)data == 0) lhs.indirectRegister(opSize, data);
    if((n2)data == 1) lhs.indirectRegisterDisplacement16(opSize, data, read16());
    if(data == 0x03) {
      auto r32 = read8();
      lhs.indirectRegisterRegister8(opSize, r32, read8());
    }
    if(data == 0x07) {
      auto r32 = read8();
      lhs.indirectRegisterRegister16(opSize, r32, read8());
    }
    if(data == 0x13 && fetch == 0xf3) {
      i16 d16 = read16();
      lhs.indirectImmediate24(opSize, pc + d16);
    }
  } break;
  case 0xc4: case 0xd4: case 0xe4: case 0xf4:
    opSourceMemory = fetch < 0xf0; opTargetMemory = !opSourceMemory;
    lhs.indirectRegisterDecrement(opSize, read8()); break;
  case 0xc5: case 0xd5: case 0xe5: case 0xf5:
    opSourceMemory = fetch < 0xf0; opTargetMemory = !opSourceMemory;
    lhs.indirectRegisterIncrement(opSize, read8()); break;
  case 0xc6: case 0xd6: case 0xe6: case 0xf6: break;
  case 0xc7: case 0xd7: case 0xe7:
    opRegister = true;
    lhs.register(opSize, read8()); break;
  case 0xf7:
    name = "ldx";
    read8(); lhs.indirectImmediate8(8, read8());
    read8(); rhs.immediate(8, read8());
    read8(); break;
  case range8(0xc8, 0xcf): opRegister = true; lhs.register3(8, fetch); break;
  case range8(0xd8, 0xdf): opRegister = true; lhs.register3(16, fetch); break;
  case range8(0xe8, 0xef): opRegister = true; lhs.register3(32, fetch); break;
  case range8(0xf8, 0xff): name = "swi"; lhs.immediate(3, (n3)fetch); break;
  }
  #undef opSize

  auto reads = [&](u32 size) -> n32 {
    if(size == 8 ) return read8 ();
    if(size == 16) return read16();
    if(size == 24) return read24();
    if(size == 32) return read32();
    return 0;
  };

  //size defined
  if(opRegister)
  switch(auto fetch = read8()) {
  case range3(0x00, 0x02): break;
  case 0x03: name = "ld"; rhs.immediate(lhs.size(), reads(lhs.size())); break;
  case 0x04: name = "push"; break;
  case 0x05: name = "pop"; break;
  case 0x06: name = "cpl"; break;
  case 0x07: name = "neg"; break;
  case 0x08: name = "mul";  rhs.immediate(lhs.size(), reads(lhs.size())); lhs.register3(16, op[0]); break;
  case 0x09: name = "muls"; rhs.immediate(lhs.size(), reads(lhs.size())); lhs.register3(16, op[0]); break;
  case 0x0a: name = "div";  rhs.immediate(lhs.size(), reads(lhs.size())); lhs.register3(16, op[0]); break;
  case 0x0b: name = "divs"; rhs.immediate(lhs.size(), reads(lhs.size())); lhs.register3(16, op[0]); break;
  case 0x0c: name = "link"; rhs.displacement(16, read16()); break;
  case 0x0d: name = "unlk"; break;
  case 0x0e: name = "bs1f"; rhs = lhs; lhs.register(8, A.id); break;
  case 0x0f: name = "bs1b"; rhs = lhs; lhs.register(8, A.id); break;
  case 0x10: name = "daa"; break;
  case 0x11: break;
  case 0x12: name = "extz"; break;
  case 0x13: name = "exts"; break;
  case 0x14: name = "paa"; break;
  case 0x15: break;
  case 0x16: name = "mirr"; break;
  case range2(0x17, 0x18): break;
  case 0x19: name = "mula"; break;
  case range2(0x1a, 0x1b): break;
  case 0x1c: name = "djnz"; rhs.displacementPC(8, read8()); break;
  case range3(0x1d, 0x1f): break;
  case 0x20: name = "andcf"; rhs.immediate(4, (n4)read8()); break;
  case 0x21: name = "orcf"; rhs.immediate(4, (n4)read8()); break;
  case 0x22: name = "xorcf"; rhs.immediate(4, (n4)read8()); break;
  case 0x23: name = "ldcf"; rhs.immediate(4, (n4)read8()); break;
  case 0x24: name = "stcf"; rhs.immediate(4, (n4)read8()); break;
  case range3(0x25, 0x27): break;
  case 0x28: name = "andcf"; rhs.register(8, A.id); break;
  case 0x29: name = "orcf"; rhs.register(8, A.id); break;
  case 0x2a: name = "xorcf"; rhs.register(8, A.id); break;
  case 0x2b: name = "ldcf"; rhs.register(8, A.id); break;
  case 0x2c: name = "stcf"; rhs.register(8, A.id); break;
  case 0x2d: break;
  case 0x2e: name = "ldc"; rhs = lhs; lhs.control(rhs.size(), read8()); break;
  case 0x2f: name = "ldc"; rhs.control(lhs.size(), read8()); break;
  case 0x30: name = "res"; rhs.immediate(4, (n4)read8()); break;
  case 0x31: name = "set"; rhs.immediate(4, (n4)read8()); break;
  case 0x32: name = "chg"; rhs.immediate(4, (n4)read8()); break;
  case 0x33: name = "bit"; rhs.immediate(4, (n4)read8()); break;
  case 0x34: name = "tset"; rhs.immediate(4, (n4)read8()); break;
  case range3(0x35, 0x37): break;
  case 0x38: name = "minc1"; rhs.immediate(16, read16()); break;
  case 0x39: name = "minc2"; rhs.immediate(16, read16()); break;
  case 0x3a: name = "minc4"; rhs.immediate(16, read16()); break;
  case 0x3b: break;
  case 0x3c: name = "mdec1"; rhs.immediate(16, read16()); break;
  case 0x3d: name = "mdec2"; rhs.immediate(16, read16()); break;
  case 0x3e: name = "mdec4"; rhs.immediate(16, read16()); break;
  case 0x3f: break;
  case range8(0x40, 0x47): name = "mul"; rhs = lhs; lhs.register3(rhs.size(), fetch); break;
  case range8(0x48, 0x4f): name = "muls"; rhs = lhs; lhs.register3(rhs.size(), fetch); break;
  case range8(0x50, 0x57): name = "div"; rhs = lhs; lhs.register3(rhs.size(), fetch); break;
  case range8(0x58, 0x5f): name = "divs"; rhs = lhs; lhs.register3(rhs.size(), fetch); break;
  case range8(0x60, 0x67): name = "inc"; rhs.immediate(4, fetch ? natural((n3)fetch) : 8_n); break;
  case range8(0x68, 0x6f): name = "dec"; rhs.immediate(4, fetch ? natural((n3)fetch) : 8_n); break;
  case range16(0x70, 0x7f): name = "scc"; rhs = lhs; lhs.condition(fetch); break;
  case range8(0x80, 0x87): name = "add"; rhs = lhs; lhs.register3(rhs.size(), fetch); break;
  case range8(0x88, 0x8f): name = "ld"; rhs = lhs; lhs.register3(rhs.size(), fetch); break;
  case range8(0x90, 0x97): name = "adc"; rhs = lhs; lhs.register3(rhs.size(), fetch); break;
  case range8(0x98, 0x9f): name = "ld"; rhs.register3(lhs.size(), fetch); break;
  case range8(0xa0, 0xa7): name = "sub"; rhs = lhs; lhs.register3(rhs.size(), fetch); break;
  case range8(0xa8, 0xaf): name = "ld"; rhs.immediate(3, (n3)fetch); break;
  case range8(0xb0, 0xb7): name = "sbb"; rhs = lhs; lhs.register3(rhs.size(), fetch); break;
  case range8(0xb8, 0xbf): name = "ex"; rhs = lhs; lhs.register3(rhs.size(), fetch); break;
  case range8(0xc0, 0xc7): name = "and"; rhs = lhs; lhs.register3(rhs.size(), fetch); break;
  case 0xc8: name = "add"; rhs.immediate(lhs.size(), reads(lhs.size())); break;
  case 0xc9: name = "adc"; rhs.immediate(lhs.size(), reads(lhs.size())); break;
  case 0xca: name = "sub"; rhs.immediate(lhs.size(), reads(lhs.size())); break;
  case 0xcb: name = "sbb"; rhs.immediate(lhs.size(), reads(lhs.size())); break;
  case 0xcc: name = "and"; rhs.immediate(lhs.size(), reads(lhs.size())); break;
  case 0xcd: name = "xor"; rhs.immediate(lhs.size(), reads(lhs.size())); break;
  case 0xce: name = "or";  rhs.immediate(lhs.size(), reads(lhs.size())); break;
  case 0xcf: name = "cp";  rhs.immediate(lhs.size(), reads(lhs.size())); break;
  case range8(0xd0, 0xd7): name = "xor"; rhs = lhs; lhs.register3(rhs.size(), fetch); break;
  case range8(0xd8, 0xdf): name = "cp"; rhs.immediate(3, (n3)fetch); break;
  case range8(0xe0, 0xe7): name = "or"; rhs = lhs; lhs.register3(rhs.size(), fetch); break;
  case 0xe8: name = "rlc"; { auto data = read8(); rhs.immediate(5, data ? natural(data) : 16_n); } break;
  case 0xe9: name = "rrc"; { auto data = read8(); rhs.immediate(5, data ? natural(data) : 16_n); } break;
  case 0xea: name = "rl";  { auto data = read8(); rhs.immediate(5, data ? natural(data) : 16_n); } break;
  case 0xeb: name = "rr";  { auto data = read8(); rhs.immediate(5, data ? natural(data) : 16_n); } break;
  case 0xec: name = "sla"; { auto data = read8(); rhs.immediate(5, data ? natural(data) : 16_n); } break;
  case 0xed: name = "sra"; { auto data = read8(); rhs.immediate(5, data ? natural(data) : 16_n); } break;
  case 0xee: name = "sll"; { auto data = read8(); rhs.immediate(5, data ? natural(data) : 16_n); } break;
  case 0xef: name = "srl"; { auto data = read8(); rhs.immediate(5, data ? natural(data) : 16_n); } break;
  case range8(0xf0, 0xf7): name = "cp"; rhs = lhs; lhs.register3(rhs.size(), fetch); break;
  case 0xf8: name = "rlc"; rhs.register(8, A.id); break;
  case 0xf9: name = "rrc"; rhs.register(8, A.id); break;
  case 0xfa: name = "rl";  rhs.register(8, A.id); break;
  case 0xfb: name = "rr";  rhs.register(8, A.id); break;
  case 0xfc: name = "sla"; rhs.register(8, A.id); break;
  case 0xfd: name = "sra"; rhs.register(8, A.id); break;
  case 0xfe: name = "sll"; rhs.register(8, A.id); break;
  case 0xff: name = "srl"; rhs.register(8, A.id); break;
  }

  //size defined
  if(opSourceMemory)
  switch(auto fetch = read8()) {
  case range4(0x00, 0x03): break;
  case 0x04: name = lhs.size() == 8 ? "push" : "pushw"; break;
  case 0x05: break;
  case 0x06: name = "rld"; rhs = lhs; lhs.register(8, A.id); break;
  case 0x07: name = "rrd"; rhs = lhs; lhs.register(8, A.id); break;
  case range8(0x08, 0x0f): break;
  case 0x10:
    name = lhs.size() == 8 ? "ldi" : "ldiw";
    lhs.indirectRegisterIncrement(32, (n3)op[0] != 5 ? XDE.id : XIX.id);
    rhs.indirectRegisterIncrement(32, (n3)op[0] != 5 ? XHL.id : XIY.id);
    break;
  case 0x11:
    name = lhs.size() == 8 ? "ldir" : "ldirw";
    lhs.indirectRegisterIncrement(32, (n3)op[0] != 5 ? XDE.id : XIX.id);
    rhs.indirectRegisterIncrement(32, (n3)op[0] != 5 ? XHL.id : XIY.id);
    break;
  case 0x12:
    name = lhs.size() == 8 ? "ldd" : "lddw";
    lhs.indirectRegisterIncrement(32, (n3)op[0] != 5 ? XDE.id : XIX.id);
    rhs.indirectRegisterIncrement(32, (n3)op[0] != 5 ? XHL.id : XIY.id);
    break;
  case 0x13:
    name = lhs.size() == 8 ? "lddr" : "lddrw";
    lhs.indirectRegisterIncrement(32, (n3)op[0] != 5 ? XDE.id : XIX.id);
    rhs.indirectRegisterIncrement(32, (n3)op[0] != 5 ? XHL.id : XIY.id);
    break;
  case 0x14:
    name = lhs.size() == 8 ? "cpi" : "cpiw";
    lhs.indirectRegister3Increment(32, op[0]);
    rhs.register(lhs.size(), lhs.size() == 8 ? A.id : WA.id);
    break;
  case 0x15:
    name = lhs.size() == 8 ? "cpir" : "cpirw";
    lhs.indirectRegister3Increment(32, op[0]);
    rhs.register(lhs.size(), lhs.size() == 8 ? A.id : WA.id);
    break;
  case 0x16:
    name = lhs.size() == 8 ? "cpd" : "cpdw";
    lhs.indirectRegister3Increment(32, op[0]);
    rhs.register(lhs.size(), lhs.size() == 8 ? A.id : WA.id);
    break;
  case 0x17:
    name = lhs.size() == 8 ? "cpdr" : "cpdrw";
    lhs.indirectRegister3Increment(32, op[0]);
    rhs.register(lhs.size(), lhs.size() == 8 ? A.id : WA.id);
    break;
  case 0x18: break;
  case 0x19: name = "ld"; rhs = lhs; lhs.indirectImmediate16(16, read16()); break;
  case range6(0x1a, 0x1f): break;
  case range8(0x20, 0x27): name = "ld"; rhs = lhs; lhs.register3(rhs.size(), fetch); break;
  case range8(0x28, 0x2f): break;
  case range8(0x30, 0x37): name = "ex"; rhs.register3(lhs.size(), fetch); break;
  case 0x38: name = "add"; rhs.immediate(lhs.size(), reads(lhs.size())); break;
  case 0x39: name = "adc"; rhs.immediate(lhs.size(), reads(lhs.size())); break;
  case 0x3a: name = "sub"; rhs.immediate(lhs.size(), reads(lhs.size())); break;
  case 0x3b: name = "sbb"; rhs.immediate(lhs.size(), reads(lhs.size())); break;
  case 0x3c: name = "and"; rhs.immediate(lhs.size(), reads(lhs.size())); break;
  case 0x3d: name = "xor"; rhs.immediate(lhs.size(), reads(lhs.size())); break;
  case 0x3e: name = "or";  rhs.immediate(lhs.size(), reads(lhs.size())); break;
  case 0x3f: name = "cp";  rhs.immediate(lhs.size(), reads(lhs.size())); break;
  case range8(0x40, 0x47): name = "mul"; rhs = lhs; lhs.register3(rhs.size(), fetch); break;
  case range8(0x48, 0x4f): name = "muls"; rhs = lhs; lhs.register3(rhs.size(), fetch); break;
  case range8(0x50, 0x57): name = "div"; rhs = lhs; lhs.register3(rhs.size(), fetch); break;
  case range8(0x58, 0x5f): name = "divs"; rhs = lhs; lhs.register3(rhs.size(), fetch); break;
  case range8(0x60, 0x67): name = "inc"; rhs.immediate(4, (n3)fetch ? natural((n3)fetch) : 8_n); break;
  case range8(0x68, 0x6f): name = "dec"; rhs.immediate(4, (n3)fetch ? natural((n3)fetch) : 8_n); break;
  case range8(0x70, 0x77): break;
  case 0x78: name = "rlc"; break;
  case 0x79: name = "rrc"; break;
  case 0x7a: name = "rl";  break;
  case 0x7b: name = "rr";  break;
  case 0x7c: name = "sla"; break;
  case 0x7d: name = "sra"; break;
  case 0x7e: name = "sll"; break;
  case 0x7f: name = "srl"; break;
  case range8(0x80, 0x87): name = "add"; rhs = lhs; lhs.register3(rhs.size(), fetch); break;
  case range8(0x88, 0x8f): name = "add"; rhs.register3(lhs.size(), fetch); break;
  case range8(0x90, 0x97): name = "adc"; rhs = lhs; lhs.register3(rhs.size(), fetch); break;
  case range8(0x98, 0x9f): name = "adc"; rhs.register3(lhs.size(), fetch); break;
  case range8(0xa0, 0xa7): name = "sub"; rhs = lhs; lhs.register3(rhs.size(), fetch); break;
  case range8(0xa8, 0xaf): name = "sub"; rhs.register3(lhs.size(), fetch); break;
  case range8(0xb0, 0xb7): name = "sbb"; rhs = lhs; lhs.register3(rhs.size(), fetch); break;
  case range8(0xb8, 0xbf): name = "sbb"; rhs.register3(lhs.size(), fetch); break;
  case range8(0xc0, 0xc7): name = "and"; rhs = lhs; lhs.register3(rhs.size(), fetch); break;
  case range8(0xc8, 0xcf): name = "and"; rhs.register3(lhs.size(), fetch); break;
  case range8(0xd0, 0xd7): name = "xor"; rhs = lhs; lhs.register3(rhs.size(), fetch); break;
  case range8(0xd8, 0xdf): name = "xor"; rhs.register3(lhs.size(), fetch); break;
  case range8(0xe0, 0xe7): name = "or"; rhs = lhs; lhs.register3(rhs.size(), fetch); break;
  case range8(0xe8, 0xef): name = "or"; rhs.register3(lhs.size(), fetch); break;
  case range8(0xf0, 0xf7): name = "cp"; rhs = lhs; lhs.register3(rhs.size(), fetch); break;
  case range8(0xf8, 0xff): name = "cp"; rhs.register3(lhs.size(), fetch); break;
  }

  //size undefined
  if(opTargetMemory)
  switch(auto fetch = read8()) {
  case 0x00: name = "ld"; lhs.size(8); rhs.immediate(8, read8()); break;
  case 0x01: break;
  case 0x02: name = "ld"; lhs.size(16); rhs.immediate(16, read16()); break;
  case 0x03: break;
  case 0x04: name = "pop"; lhs.size(8); break;
  case 0x05: break;
  case 0x06: name = "pop"; lhs.size(16); break;
  case range13(0x07, 0x13): break;
  case 0x14: name = "ld"; lhs.size(8); rhs.indirectImmediate16(16, read16()); break;
  case 0x15: break;
  case 0x16: name = "ld"; lhs.size(16); rhs.indirectImmediate16(16, read16()); break;
  case range9(0x17, 0x1f): break;
  case range8(0x20, 0x27): name = "lda"; lhs.size(16); rhs = lhs; lhs.register3(rhs.size(), fetch); break;
  case 0x28: name = "andcf"; lhs.size(8); rhs.register3(8, A.id); break;
  case 0x29: name = "orcf";  lhs.size(8); rhs.register3(8, A.id); break;
  case 0x2a: name = "xorcf"; lhs.size(8); rhs.register3(8, A.id); break;
  case 0x2b: name = "ldcf";  lhs.size(8); rhs.register3(8, A.id); break;
  case 0x2c: name = "stcf";  lhs.size(8); rhs.register3(8, A.id); break;
  case range3(0x2d, 0x2f): break;
  case range8(0x30, 0x37): name = "lda"; lhs.size(32); rhs = lhs; lhs.register3(rhs.size(), fetch); break;
  case range8(0x38, 0x3f): break;
  case range8(0x40, 0x47): name = "ld"; lhs.size(8); rhs.register3(lhs.size(), fetch); break;
  case range8(0x48, 0x4f): break;
  case range8(0x50, 0x57): name = "ld"; lhs.size(16); rhs.register3(lhs.size(), fetch); break;
  case range8(0x58, 0x5f): break;
  case range8(0x60, 0x67): name = "ld"; lhs.size(32); rhs.register3(lhs.size(), fetch); break;
  case range8(0x68, 0x6f): break;
  case range8(0x70, 0x77): break;
  case range8(0x78, 0x7f): break;
  case range8(0x80, 0x87): name = "andcf"; lhs.size(8); rhs.immediate(3, (n3)fetch); break;
  case range8(0x88, 0x8f): name = "orcf"; lhs.size(8); rhs.immediate(3, (n3)fetch); break;
  case range8(0x90, 0x97): name = "xorcf"; lhs.size(8); rhs.immediate(3, (n3)fetch); break;
  case range8(0x98, 0x9f): name = "ldcf"; lhs.size(8); rhs.immediate(3, (n3)fetch); break;
  case range8(0xa0, 0xa7): name = "stcf"; lhs.size(8); rhs.immediate(3, (n3)fetch); break;
  case range8(0xa8, 0xaf): name = "tset"; lhs.size(8); rhs.immediate(3, (n3)fetch); break;
  case range8(0xb0, 0xb7): name = "res"; lhs.size(8); rhs.immediate(3, (n3)fetch); break;
  case range8(0xb8, 0xbf): name = "set"; lhs.size(8); rhs.immediate(3, (n3)fetch); break;
  case range8(0xc0, 0xc7): name = "chg"; lhs.size(8); rhs.immediate(3, (n3)fetch); break;
  case range8(0xc8, 0xcf): name = "bit"; lhs.size(8); rhs.immediate(3, (n3)fetch); break;
  case range16(0xd0, 0xdf): name = "jp"; lhs.size(32); rhs = lhs; lhs.condition(fetch); break;
  case range16(0xe0, 0xef): name = "call"; lhs.size(32); rhs = lhs; lhs.condition(fetch); break;
  case range16(0xf0, 0xff): name = "ret"; lhs.condition(fetch); break;
  }

  auto operand = [&](Operand& operand) -> string {
    static const string registers8[] = {
      "ra0", "rw0", "qa0", "qw0", "rc0", "rb0", "qc0", "qb0", "re0", "rd0", "qe0", "qd0", "rl0", "rh0", "ql0", "qh0",   //00-1f
      "ra1", "rw1", "qa1", "qw1", "rc1", "rb1", "qc1", "qb1", "re1", "rd1", "qe1", "qd1", "rl1", "rh1", "ql1", "qh1",   //10-1f
      "ra2", "rw2", "qa2", "qw2", "rc2", "rb2", "qc2", "qb2", "re2", "rd2", "qe2", "qd2", "rl2", "rh2", "ql2", "qh2",   //20-2f
      "ra3", "rw3", "qa3", "qw3", "rc3", "rb3", "qc3", "qb3", "re3", "rd3", "qe3", "qd3", "rl3", "rh3", "ql3", "qh3",   //30-3f
       "a'",  "w'", "qa'", "qw'",  "c'",  "b'", "qc'", "qb'",  "e'",  "d'", "qe'", "qd'",  "l'",  "h'", "ql'", "qh'",   //d0-df
       "a",   "w",  "qa",  "qw",   "c",   "b",  "qc",  "qb",   "e",   "d",  "qe",  "qd",   "l",   "h",  "ql",  "qh",    //e0-ef
      "ixl", "ixh", "qixl","qixh","iyl", "iyh", "qiyl","qiyh","izl", "izh", "qizl","qizh","spl", "sph", "qspl","qsph",  //f0-ff
    };
    auto register8 = [&](n8 register) -> string {
      if(register <  0x40) return registers8[register];
      if(register >= 0xd0) return registers8[(register - 0xd0 >> 0) + 0x40];
      return "rb?";
    };

    static const string registers16[] = {
      "rwa0", "qwa0", "rbc0", "qbc0", "rde0", "qde0", "rhl0", "qhl0",  //00-0f
      "rwa1", "qwa1", "rbc1", "qbc1", "rde1", "qde1", "rhl1", "qhl1",  //10-1f
      "rwa2", "qwa2", "rbc2", "qbc2", "rde2", "qde2", "rhl2", "qhl2",  //20-2f
      "rwa3", "qwa3", "rbc3", "qbc3", "rde3", "qde3", "rhl3", "qhl3",  //30-3f
       "wa'", "qwa'",  "bc'", "qbc'",  "de'", "qde'",  "hl'", "qhl'",  //d0-df
       "wa",  "qwa",   "bc",  "qbc",   "de",  "qde",   "hl",  "qhl",   //e0-ef
       "ix",  "qix",   "iy",  "qiy",   "iz",  "qiz",   "sp",  "qsp",   //f0-ff
    };
    auto register16 = [&](n8 register) -> string {
      if(register <  0x40) return registers16[register >> 1];
      if(register >= 0xd0) return registers16[(register - 0xd0 >> 1) + 0x20];
      return "rw?";
    };

    static const string registers32[] = {
      "xwa0", "xbc0", "xde0", "xhl0",  //00-0f
      "xwa1", "xbc1", "xde1", "xhl1",  //10-1f
      "xwa2", "xbc2", "xde2", "xhl2",  //20-2f
      "xwa3", "xbc3", "xde3", "xhl3",  //30-3f
      "xwa'", "xbc'", "xde'", "xhl'",  //d0-df
      "xwa",  "xbc",  "xde",  "xhl",   //e0-ef
      "xix",  "xiy",  "xiz",  "xsp",   //f0-ff
    };
    auto register32 = [&](n8 register) -> string {
      if(register <  0x40) return registers32[register >> 2];
      if(register >= 0xd0) return registers32[(register - 0xd0 >> 2) + 0x10];
      return "rl?";
    };

    auto registerSize = [&](natural size, n8 register) -> string {
      if(size == 8 ) return register8 (register);
      if(size == 16) return register16(register);
      if(size == 32) return register32(register);
      return "r??";
    };

    //there are no names for byte-accesses to control registers
    auto control8 = [&](n8 register) -> string {
      return {"c", hex(register, 2L)};
    };

    static string controls16[] = {
      "dmas0l", "dmas0h", "dmas1l", "dmas1h", "dmas2l", "dmas2h", "dmas3l", "dmas3h",  //00-0f
      "dmad0l", "dmad0h", "dmad1l", "dmad1h", "dmad2l", "dmad2h", "dmad3l", "dmad3h",  //10-1f
      "dmac0",  "dmac0h", "dmac1",  "dmac1h", "dmac2",  "dmac2h", "dmac3",  "dmac3h",  //20-2f
    };
    auto control16 = [&](n8 register) -> string {
      if(register <  0x30) return controls16[register >> 1];
      if(register >= 0x3c || register <= 0x3d) return "intnest";
      return "cw?";
    };

    static string controls32[] = {
      "dmas0", "dmas1", "dmas2", "dmas3",  //00-0f
      "dmad0", "dmad1", "dmad2", "dmad3",  //10-1f
      "dmam0", "dmam1", "dmam2", "dmam3",  //20-2f
    };
    auto control32 = [&](n8 register) -> string {
      if(register <  0x30) return controls32[register >> 2];
      if(register >= 0x3c && register <= 0x3f) return "intnest";
      return "cl?";
    };

    auto controlSize = [&](natural size, n8 register) -> string {
      if(size == 8 ) return control8 (register);
      if(size == 16) return control16(register);
      if(size == 32) return control32(register);
      return "c??";
    };

    if(operand.mode() == Text) return operand.text();
    if(operand.mode() == Condition) {
      static const string conditions[] = {
        "f", "lt", "le", "ule", "ov",  "mi", "eq", "ult",
        "t", "ge", "gt", "ugt", "nov", "pl", "ne", "uge",
      };
      return conditions[operand.condition()];
    }
    if(operand.mode() == Register) return registerSize(operand.size(), operand.register());
    if(operand.mode() == Control) return controlSize(operand.size(), operand.register());
    if(operand.mode() == Immediate) {
      if(operand.size() <=  7) return {operand.immediate()};
      if(operand.size() ==  8) return {"0x", hex(operand.immediate(), 2L)};
      if(operand.size() == 16) return {"0x", hex(operand.immediate(), 4L)};
      if(operand.size() == 24) return {"0x", hex(operand.immediate(), 6L)};
      if(operand.size() == 32) return {"0x", hex(operand.immediate(), 8L)};
    }
    if(operand.mode() == Displacement) {
      if(operand.size() ==  8) {
        integer displacement = (i8)operand.displacement();
        if(displacement <  0) return {"-0x", hex(-displacement, 2L)};
        if(displacement >= 0) return {"+0x", hex(+displacement, 2L)};
      }
      if(operand.size() == 16) {
        integer displacement = (i8)operand.displacement();
        if(displacement <  0) return {"-0x", hex(-displacement, 4L)};
        if(displacement >= 0) return {"+0x", hex(+displacement, 4L)};
      }
    }
    if(operand.mode() == DisplacementPC) {
      if(operand.size() ==  8) {
        natural displacement = pc + (i8)operand.displacement();
        return {"0x", hex(displacement, 6L)};
      }
      if(operand.size() == 16) {
        natural displacement = pc + (i16)operand.displacement();
        return {"0x", hex(displacement, 6L)};
      }
    }
    if(operand.mode() == IndirectRegister) return {"(", registerSize(operand.size(), operand.register()), ")"};
    if(operand.mode() == IndirectRegisterDecrement) return {"(-", register32(operand.register()), ")"};
    if(operand.mode() == IndirectRegisterIncrement) return {"(", register32(operand.register()), "+)"};
    if(operand.mode() ==  IndirectRegisterRegister8) return {"(", register32(operand.register()), "+",  register8(operand.registerAdd()), ")"};
    if(operand.mode() == IndirectRegisterRegister16) return {"(", register32(operand.register()), "+", register16(operand.registerAdd()), ")"};
    if(operand.mode() ==  IndirectRegisterDisplacement8) {
      integer displacement = (i8)operand.displacement();
      if(displacement == 0) return {"(", register32(operand.register()), ")"};
      if(displacement <  0) return {"(", register32(operand.register()), "-0x", hex(-displacement, 2L), ")"};
      if(displacement >  0) return {"(", register32(operand.register()), "+0x", hex(+displacement, 2L), ")"};
    }
    if(operand.mode() == IndirectRegisterDisplacement16) {
      integer displacement = (i16)operand.displacement();
      if(displacement == 0) return {"(", register32(operand.register()), ")"};
      if(displacement <  0) return {"(", register32(operand.register()), "-0x", hex(-displacement, 4L), ")"};
      if(displacement >  0) return {"(", register32(operand.register()), "+0x", hex(+displacement, 4L), ")"};
    }
    if(operand.mode() ==  IndirectImmediate8) return {"(0x", hex(operand.immediate(), 2L), ")"};
    if(operand.mode() == IndirectImmediate16) return {"(0x", hex(operand.immediate(), 4L), ")"};
    if(operand.mode() == IndirectImmediate24) return {"(0x", hex(operand.immediate(), 6L), ")"};
    return {};
  };

  //omit true condition operand
  if(lhs.mode() == Condition && lhs.condition() == 8) {
    lhs = rhs;
    rhs.null();
  }

  if(name) {
    output.append(pad(name, -6));
    if(lhs) {
      output.append(operand(lhs));
      if(rhs) {
        output.append(",", operand(rhs));
      }
    }
  } else {
    output.append(pad("???", -6));
    for(auto byte : range(ops)) output.append("0x", hex(op[byte], 2L), ",");
    output.trimRight(",", 1L);
  }

  return pad(output, -48);
}

NALL_NOINLINE auto TLCS900H::disassembleContext() -> string {
  string output;
  output.append("XWA:", hex(r.xwa[RFP].l.l0, 8L), " ");
  output.append("XBC:", hex(r.xbc[RFP].l.l0, 8L), " ");
  output.append("XDE:", hex(r.xde[RFP].l.l0, 8L), " ");
  output.append("XHL:", hex(r.xhl[RFP].l.l0, 8L), " ");
  output.append("XIX:", hex(r.xix.l.l0, 8L), " ");
  output.append("XIY:", hex(r.xiy.l.l0, 8L), " ");
  output.append("XIZ:", hex(r.xiz.l.l0, 8L), " ");
  output.append("XSP:", hex(r.xsp.l.l0, 8L), " ");
  output.append("IFF:", IFF, " ");
  output.append("RFP:", RFP, " ");
  output.append(SF ? "S" : "s");
  output.append(ZF ? "Z" : "z");
  output.append(HF ? "H" : "h");
  output.append(VF ? "V" : "v");
  output.append(NF ? "N" : "n");
  output.append(CF ? "C" : "c");
  return output;
}
