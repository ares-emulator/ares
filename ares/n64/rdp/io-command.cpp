static const vector<string> commandRegisterNames = {
  "DPC_START",
  "DPC_END",
  "DPC_CURRENT",
  "DPC_STATUS",
  "DPC_CLOCK",
  "DPC_BUFBUSY",
  "DPC_PIPEBUSY",
  "DPC_TMEM",
};

auto RDP::readCommand(u32 address) -> u32 {
  address = (address & 0xfffff) >> 2;
  u32 data = 0;

  if(address == 0) {
    //DPC_START
  }

  if(address == 1) {
    //DPC_END
  }

  if(address == 2) {
    //DPC_CURRENT
  }

  if(address == 3) {
    //DPC_STATUS
  }

  if(address == 4) {
    //DPC_CLOCK
  }

  if(address == 5) {
    //DPC_BUFBUSY
  }

  if(address == 6) {
    //DPC_PIPEBUSY
  }

  if(address == 7) {
    //DPC_TMEM
  }

  print("* ", commandRegisterNames(address, "DPC_UNKNOWN"), " => ", hex(data, 8L), "\n");
  return data;
}

auto RDP::writeCommand(u32 address, u32 data) -> void {
  address = (address & 0xfffff) >> 2;

  if(address == 0) {
    //DPC_START
  }

  if(address == 1) {
    //DPC_END
  }

  if(address == 2) {
    //DPC_CURRENT
  }

  if(address == 3) {
    //DPC_STATUS
  }

  if(address == 4) {
    //DPC_CLOCK
  }

  if(address == 5) {
    //DPC_BUFBUSY
  }

  if(address == 6) {
    //DPC_PIPEBUSY
  }

  if(address == 7) {
    //DPC_TMEM
  }

  print("* ", commandRegisterNames(address, "DPC_UNKNOWN"), " <= ", hex(data, 8L), "\n");
}
