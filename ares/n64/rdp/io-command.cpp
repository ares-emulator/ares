static const vector<string> commandRegisterNames = {
  "DPC_START",
  "DPC_END",
  "DPC_CURRENT",
  "DPC_STATUS",
  "DPC_CLOCK",
  "DPC_BUSY",
  "DPC_PIPE_BUSY",
  "DPC_TMEM_BUSY",
};

auto RDP::readCommand(u32 address) -> u32 {
  address = (address & 0xfffff) >> 2;
  u32 data = 0;

  if(address == 0) data = rsp.getControlRegister( 8);
  if(address == 1) data = rsp.getControlRegister( 9);
  if(address == 2) data = rsp.getControlRegister(10);
  if(address == 3) data = rsp.getControlRegister(11);
  if(address == 4) data = rsp.getControlRegister(12);
  if(address == 5) data = rsp.getControlRegister(13);
  if(address == 6) data = rsp.getControlRegister(14);
  if(address == 7) data = rsp.getControlRegister(15);

print("* ", commandRegisterNames(address, "DPC_UNKNOWN"), " => ", hex(data, 8L), "\n");
  return data;
}

auto RDP::writeCommand(u32 address, u32 data) -> void {
  address = (address & 0xfffff) >> 2;

  if(address == 0) rsp.setControlRegister( 8, data);
  if(address == 1) rsp.setControlRegister( 9, data);
  if(address == 2) rsp.setControlRegister(10, data);
  if(address == 3) rsp.setControlRegister(11, data);
  if(address == 4) rsp.setControlRegister(12, data);
  if(address == 5) rsp.setControlRegister(13, data);
  if(address == 6) rsp.setControlRegister(14, data);
  if(address == 7) rsp.setControlRegister(15, data);

print("* ", commandRegisterNames(address, "DPC_UNKNOWN"), " <= ", hex(data, 8L), "\n");
}
