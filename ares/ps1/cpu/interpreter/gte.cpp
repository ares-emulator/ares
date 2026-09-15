auto CPU::CFC2(u32& rt, u8 rd) -> void {
  if(!coprocessor2Enabled()) return exception.coprocessor(2);
  stallGTE();
  load(rt, gte.getControlRegister(rd));
}

auto CPU::CTC2(cu32& rt, u8 rd) -> void {
  if(!coprocessor2Enabled()) return exception.coprocessor(2);
  scheduleGTEControlWrite(rd, rt);
}

auto CPU::LWC2(u8 rt, cu32& rs, s16 imm) -> void {
  if(!coprocessor2Enabled()) return exception.coprocessor(2);
  auto data = read<Word>(rs + imm);
  if(exception()) return;
  scheduleGTEDataWrite(rt, data);
}

auto CPU::MFC2(u32& rt, u8 rd) -> void {
  if(!coprocessor2Enabled()) return exception.coprocessor(2);
  stallGTE();
  load(rt, gte.getDataRegister(rd));
}

auto CPU::MTC2(cu32& rt, u8 rd) -> void {
  if(!coprocessor2Enabled()) return exception.coprocessor(2);
  scheduleGTEDataWrite(rd, rt);
}

auto CPU::SWC2(u8 rt, cu32& rs, s16 imm) -> void {
  if(!coprocessor2Enabled()) return exception.coprocessor(2);
  stallGTE();
  auto data = gte.getDataRegister(rt);
  write<Word>(rs + imm, data);
}
