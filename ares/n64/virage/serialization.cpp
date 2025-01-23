auto Virage::serialize(serializer& s) -> void {
  s(flash);
  s(sram);

  s(num);
  s(queueID);
  s(memSize);

  s(io.configReg[0]);
  s(io.configReg[1]);
  s(io.configReg[2]);
  s(io.configReg[3]);
  s(io.configReg[4]);
  s(io.configReg[5]);
  s(io.busy);
  s(io.loadDone);
  s(io.storeDone);
  s(io.unk30);
  s(io.command);
}
